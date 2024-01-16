#include "Core.hpp"

namespace pd
{
	std::vector < char const * > GetWindowRequiredVulkanExtensions ( SDL_Window * window )
	{
		unsigned int extensionCount;
		SDL_Vulkan_GetInstanceExtensions ( window, &extensionCount, nullptr );
		std::vector < char const * > extensions ( extensionCount );
		SDL_Vulkan_GetInstanceExtensions ( window, &extensionCount, extensions.data () );
		return extensions;
	}

	glm::vec2 GetWindowSize ( SDL_Window * window )
	{
		glm::ivec2 size;
		SDL_GetWindowSizeInPixels ( window, &size.x, &size.y );
		return size;
	}

	vk::Instance CreateInstance ( SDL_Window * window )
	{
		vk::ApplicationInfo appInfo { "Palladium", 1, "", 0, vk::enumerateInstanceVersion () };

		std::vector <char const *> layers {	"VK_LAYER_KHRONOS_validation" };
		
		auto extensions { GetWindowRequiredVulkanExtensions ( window ) };
		extensions.push_back ( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );

		vk::InstanceCreateInfo createInfo { {}, &appInfo, layers, extensions };

		return vk::createInstance ( createInfo );
	}

	vk::SurfaceKHR CreateWindowSurface ( vk::Instance instance, SDL_Window * window )
	{
		VkSurfaceKHR surface;
		SDL_Vulkan_CreateSurface ( window, instance, &surface );
		return surface;
	}

	VkBool32 debugCallback (
		VkDebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT              messageTypes,
		const VkDebugUtilsMessengerCallbackDataEXT * pCallbackData,
		void *
	)
	{
		std::cout << "VULKAN ";
		
		switch ( messageSeverity )
		{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:		std::cout << "INFO";  break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:		std::cout << "ERROR"; break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:	std::cout << "VERBOSE"; break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:	std::cout << "WARNING"; break;
		}

		std::cout << ": " << pCallbackData->pMessage << '\n' << std::endl;
		
		return VK_FALSE;
	}

	vk::DebugUtilsMessengerEXT CreateDebugUtilsMessenger ( vk::Instance instance )
	{
		vk::DebugUtilsMessengerCreateInfoEXT createInfo {
			{},
			  vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
			| vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo
			| vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning,
			//| vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose,

			  vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
			| vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding
			| vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
			| vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,

			debugCallback,
			nullptr
		};

		return instance.createDebugUtilsMessengerEXT ( createInfo );
	}

	vk::PhysicalDevice SelectPhysicalDevice ( vk::Instance instance, vk::SurfaceKHR surface )
	{
		for ( auto const & physicalDevice : instance.enumeratePhysicalDevices () )
		{
			int queueFamilyIndex { 0 };
			for ( auto const & queueFamilyProperties : physicalDevice.getQueueFamilyProperties () )
			{
				if ( physicalDevice.getSurfaceSupportKHR ( queueFamilyIndex, surface ) )
					return physicalDevice;
					
				++queueFamilyIndex;
			}
		}

		return {};
	}

	void CreateDevice ( vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface,
		vk::Device & device, DeviceQueues & queueConfiguration )
	{
		// Search for an all in one queue family
		int allInOneQueueFamilyIndex { -1 };
		uint32_t queueFamilyIndex { 0 };

		for ( auto const & queueFamilyProperties : physicalDevice.getQueueFamilyProperties () )
		{
			if ( queueFamilyProperties.queueFlags & vk::QueueFlagBits::eGraphics )
				if ( physicalDevice.getSurfaceSupportKHR ( queueFamilyIndex, surface ) )
					allInOneQueueFamilyIndex = queueFamilyIndex;

			++queueFamilyIndex;
		}

		if ( allInOneQueueFamilyIndex != -1 )
		{
			std::vector <float> queuePriorities { 1.0f };
			std::vector <vk::DeviceQueueCreateInfo> queueCreateInfos { { {}, static_cast <uint32_t> ( allInOneQueueFamilyIndex ), queuePriorities } };
			std::vector < char const * > extensions { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
			vk::DeviceCreateInfo createInfo ( {}, queueCreateInfos, {}, extensions, {} );
			device = physicalDevice.createDevice ( createInfo );

			queueConfiguration.graphicsQueueFamilyIndex
				= queueConfiguration.presentationQueueFamilyIndex
				= queueConfiguration.transferQueueFamilyIndex
				= allInOneQueueFamilyIndex;

			queueConfiguration.graphicsQueue
				= queueConfiguration.transferQueue
				= queueConfiguration.presentationQueue
				= device.getQueue ( allInOneQueueFamilyIndex, 0 );

			return;
		}

		std::cerr << "Failed to create device" << std::endl;
		throw std::runtime_error { "Failed to create device" };
	}
	
	vk::SurfaceFormatKHR SelectSurfaceFormat ( vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface )
	{
		return physicalDevice.getSurfaceFormatsKHR ( surface ) [0];
	}

	vk::SwapchainKHR CreateSwapchain (
		vk::PhysicalDevice physicalDevice, 
		vk::Device device, 
		vk::SurfaceKHR surface,
		vk::SurfaceFormatKHR const & format,
		glm::vec2 const & size )
	{
		vk::SwapchainCreateInfoKHR createInfo
		{
			{},
			surface,
			3,
			format.format,
			format.colorSpace,
			{ static_cast <uint32_t> ( size.x ), static_cast < uint32_t > ( size.y ) },
			1,
			vk::ImageUsageFlagBits::eColorAttachment,
			vk::SharingMode::eExclusive,
			{},
			vk::SurfaceTransformFlagBitsKHR::eIdentity,
			vk::CompositeAlphaFlagBitsKHR::eOpaque,
			vk::PresentModeKHR::eImmediate,
			0,
			{}
		};

		return device.createSwapchainKHR ( createInfo );
	}

	vk::RenderPass CreateRenderPass ( vk::Device device, vk::Format outputFormat )
	{
		vk::AttachmentDescription outputAttachment {
			{},
			outputFormat,
			vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eStore,
			vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::ePresentSrcKHR
		};

		vk::AttachmentReference outputAttachmentReference { 0, vk::ImageLayout::eColorAttachmentOptimal };

		auto subpassColorAttachments = { outputAttachmentReference };

		vk::SubpassDescription subpass
		{
			{},
			vk::PipelineBindPoint::eGraphics,
			{}, // Input attachments
			subpassColorAttachments
		};

		auto attachments = { outputAttachment };
		auto subpasses = { subpass };

		vk::RenderPassCreateInfo createInfo
		{
			{},
			attachments,
			subpass,
			{} // Subpass dependencies
		};

		return device.createRenderPass ( createInfo );
	}
	
	std::vector <vk::ImageView> CreateSwapchainImageViews ( vk::Device device, vk::SwapchainKHR swapchain, vk::Format format )
	{
		auto images { device.getSwapchainImagesKHR ( swapchain ) };

		std::vector <vk::ImageView> imageViews;
		imageViews.reserve ( images.size () );
		
		for ( auto const & image : images )
		{
			vk::ImageViewCreateInfo createInfo
			{
				{},
				image,
				vk::ImageViewType::e2D,
				format,
				{ vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity },
				{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
			};

			imageViews.push_back ( device.createImageView ( createInfo ) );
		}

		return imageViews;
	}

	std::vector <vk::Framebuffer> CreateFramebuffers ( vk::Device device, vk::RenderPass renderPass, std::vector <vk::ImageView> attachments, glm::vec2 const & size )
	{
		std::vector <vk::Framebuffer> framebuffers;
		framebuffers.reserve ( attachments.size () );

		for ( auto const & attachment : attachments )
		{
			auto attachments = { attachment };

			vk::FramebufferCreateInfo createInfo
			{
				{},
				renderPass,
				attachments,
				static_cast <uint32_t> ( size.x ),
				static_cast <uint32_t> ( size.y ),
				1
			};

			framebuffers.push_back ( device.createFramebuffer ( createInfo ) );
		}

		return framebuffers;
	}

}