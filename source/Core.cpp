#include "Core.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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

	glm::vec2 GetMousePosition ()
	{
		int x, y;
		SDL_GetMouseState ( &x, &y );
		return { x, y };
	}

	vk::Instance CreateInstance ( SDL_Window * window )
	{
		auto instanceVersion { vk::enumerateInstanceVersion () };

		vk::ApplicationInfo appInfo { "Palladium", 1, "", 0, instanceVersion };

		std::vector <char const *> layers { "VK_LAYER_KHRONOS_validation" };

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
			std::vector <vk::DeviceQueueCreateInfo> queueCreateInfos { { {}, static_cast < uint32_t > ( allInOneQueueFamilyIndex ), queuePriorities } };
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
		return physicalDevice.getSurfaceFormatsKHR ( surface ) [ 0 ];
	}

	vk::SwapchainKHR CreateSwapchain (
		vk::PhysicalDevice physicalDevice,
		vk::Device device,
		vk::SurfaceKHR surface,
		vk::SurfaceFormatKHR const & format,
		glm::vec2 const & size, 
		vk::SwapchainKHR oldSwapchain )
	{
		static_cast <void> ( physicalDevice.getSurfaceCapabilitiesKHR ( surface ) );

		vk::SwapchainCreateInfoKHR createInfo
		{
			{},
			surface,
			3,
			format.format,
			format.colorSpace,
			{ static_cast < uint32_t > ( size.x ), static_cast < uint32_t > ( size.y ) },
			1,
			vk::ImageUsageFlagBits::eColorAttachment,
			vk::SharingMode::eExclusive,
			{},
			vk::SurfaceTransformFlagBitsKHR::eIdentity,
			vk::CompositeAlphaFlagBitsKHR::eOpaque,
			vk::PresentModeKHR::eImmediate,
			0,
			oldSwapchain
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
		
		vk::AttachmentDescription depthAttachment {
			{},
			vk::Format::eD32Sfloat,
			vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eDontCare,
			vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eDepthStencilAttachmentOptimal
		};

		vk::AttachmentReference outputAttachmentReference { 0, vk::ImageLayout::eColorAttachmentOptimal };
		vk::AttachmentReference depthAttachmentReference { 1, vk::ImageLayout::eDepthStencilAttachmentOptimal };

		auto subpassColorAttachments = { outputAttachmentReference };

		vk::SubpassDescription subpass
		{
			{},
			vk::PipelineBindPoint::eGraphics,
			{}, // Input attachments
			subpassColorAttachments,
			{},
			&depthAttachmentReference
		};

		auto attachments = { outputAttachment, depthAttachment };
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

	std::vector <vk::Framebuffer> CreateFramebuffers ( 
		vk::Device device, vk::RenderPass renderPass, 
		std::vector <vk::ImageView> attachments, vk::ImageView depthAttachment, glm::vec2 const & size )
	{
		std::vector <vk::Framebuffer> framebuffers;
		framebuffers.reserve ( attachments.size () );

		for ( auto const & attachment : attachments )
		{
			auto attachments = { attachment, depthAttachment };

			vk::FramebufferCreateInfo createInfo
			{
				{},
				renderPass,
				attachments,
				static_cast < uint32_t > ( size.x ),
				static_cast < uint32_t > ( size.y ),
				1
			};

			framebuffers.push_back ( device.createFramebuffer ( createInfo ) );
		}

		return framebuffers;
	}

	vk::PipelineLayout CreatePipelineLayout ( 
		vk::Device device, 
		std::vector <vk::DescriptorSetLayout> const & descriptorSetLayouts, 
		std::vector <vk::PushConstantRange> const & pushConstantRanges )
	{
		vk::PipelineLayoutCreateInfo createInfo
		{
			{},
			descriptorSetLayouts,
			pushConstantRanges
		};

		return device.createPipelineLayout ( createInfo );
	}

	vk::ShaderModule CreateShaderModuleFromFile ( vk::Device device, std::string const & filePath )
	{
		std::ifstream file { filePath, std::ios::ate | std::ios::binary };
		auto size { file.tellg () };
		file.seekg ( 0 );
		std::string data { ( std::ostringstream {} << file.rdbuf () ).str () };

		std::vector <uint32_t> code ( size );
		std::memcpy ( code.data (), data.data (), data.size () );

		vk::ShaderModuleCreateInfo createInfo { {}, static_cast < size_t > ( size ), code.data () };
		return device.createShaderModule ( createInfo );
	}

	vk::Pipeline CreateGraphicsPipeline ( GraphicsPipelineCreateInfo const & info )
	{
		vk::ShaderModule vertexShader { CreateShaderModuleFromFile ( info.device, "shader/build/shader.spv.vert" ) };
		vk::ShaderModule fragmentShader { CreateShaderModuleFromFile ( info.device, "shader/build/shader.spv.frag" ) };

		std::vector <vk::PipelineShaderStageCreateInfo> shaderStages
		{
			{ {}, vk::ShaderStageFlagBits::eVertex, vertexShader, "main" },
			{ {}, vk::ShaderStageFlagBits::eFragment, fragmentShader, "main" }
		};

		std::vector <vk::VertexInputBindingDescription> vertexBindings { { 0, sizeof ( float ) * ( 3 + 2 ), vk::VertexInputRate::eVertex } };
		
		std::vector <vk::VertexInputAttributeDescription> vertexAttributes { 
			{ 0, 0, vk::Format::eR32G32B32Sfloat, 0 }, 
			{ 1, 0, vk::Format::eR32G32Sfloat, sizeof ( float ) * 3 } };

		vk::PipelineVertexInputStateCreateInfo vertexInputState
		{ {}, vertexBindings, vertexAttributes };

		vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState
		{ {}, vk::PrimitiveTopology::eTriangleList, VK_FALSE };

		std::vector <vk::Viewport> viewports { { 0, 0, 1280, 720, 0.0f, 1.0f } };
		std::vector <vk::Rect2D> scissors { { { 0, 0 }, { 1280, 720 } } };

		vk::PipelineViewportStateCreateInfo viewportState { {}, viewports, scissors };

		vk::PipelineRasterizationStateCreateInfo rasterizationState
		{
			{},
			VK_FALSE,
			VK_FALSE,
			vk::PolygonMode::eFill,
			vk::CullModeFlagBits::eNone,
			{},
			{},
			{},
			{},
			{},
			1.0f
		};

		vk::PipelineMultisampleStateCreateInfo multisampleState
		{
			{},
			vk::SampleCountFlagBits::e1,
			VK_FALSE
		};

		std::vector <vk::PipelineColorBlendAttachmentState> colorBlendAttachmentStates
		{
			{
				VK_FALSE,
				{},
				{},
				{},
				{},
				{},
				{},
				vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
			}
		};

		vk::PipelineColorBlendStateCreateInfo colorBlendState
		{
			{},
			VK_FALSE,
			{},
			colorBlendAttachmentStates
		};

		std::vector <vk::DynamicState> dynamicStates { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
		vk::PipelineDynamicStateCreateInfo dynamicState { {}, dynamicStates };

		vk::PipelineDepthStencilStateCreateInfo depthStencilState
		{
			{},
			VK_TRUE,
			VK_TRUE,
			vk::CompareOp::eLess,
			VK_FALSE,
			VK_FALSE
		};

		vk::GraphicsPipelineCreateInfo createInfo
		{
			{},
			shaderStages,
			&vertexInputState,
			&inputAssemblyState,
			nullptr,
			&viewportState,
			&rasterizationState,
			&multisampleState,
			&depthStencilState,
			&colorBlendState,
			&dynamicState,
			info.pipelineLayout,
			info.renderPass,
			info.subpass,
		};

		auto pipeline { info.device.createGraphicsPipeline ( {}, createInfo ).value };

		info.device.destroy ( vertexShader );
		info.device.destroy ( fragmentShader );

		return pipeline;
	}

	void Submit (
		vk::Queue queue,
		std::vector <vk::CommandBuffer> const & commandBuffers,
		vk::Fence signalFence,
		std::vector <vk::Semaphore> signalSemaphores,
		std::vector <vk::Semaphore> const & waitSemaphores,
		std::vector <vk::PipelineStageFlags> waitStages
	)
	{
		vk::SubmitInfo info
		{
			waitSemaphores,
			waitStages,
			commandBuffers,
			signalSemaphores
		};

		queue.submit ( { info }, signalFence );

	}

	vk::Result Present ( vk::Queue queue, vk::SwapchainKHR swapchain, uint32_t imageIndex, vk::Semaphore waitSemaphore )
	{
		auto waitSemaphores = { waitSemaphore };
		auto swapchains = { swapchain };
		std::vector <uint32_t> imageIndices { imageIndex };

		vk::PresentInfoKHR presentInfo
		{
			waitSemaphores,
			swapchains,
			imageIndices
		};

		return queue.presentKHR ( presentInfo );
	}

	vk::Buffer CreateBuffer ( vk::Device device, BufferUsages usage, vk::DeviceSize size )
	{
		vk::BufferUsageFlags vkUsage;

		switch ( usage )
		{
		case BufferUsages::vertexBuffer:
			vkUsage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;
			break;

		case BufferUsages::indexBuffer:
			vkUsage = vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst;
			break;

		case BufferUsages::uniformBuffer:
			vkUsage = vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst;
			break;

		case BufferUsages::stagingBuffer:
			vkUsage = vk::BufferUsageFlagBits::eTransferSrc;
			break;
		}

		return device.createBuffer ( { {}, size, vkUsage } );
	}

	vk::DeviceMemory AllocateMemory ( vk::PhysicalDevice physicalDevice, vk::Device device, MemoryTypes type, vk::DeviceSize size )
	{
		bool typeFound { false };
		uint32_t typeIndex { 0 };
		auto memoryProperties { physicalDevice.getMemoryProperties () };

		switch ( type )
		{
		case MemoryTypes::deviceLocal:
			typeIndex = 0;
			for ( auto const & type : memoryProperties.memoryTypes )
			{
				if ( type.propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal )
				{
					if ( memoryProperties.memoryHeaps [ type.heapIndex ].size >= size )
					{
						typeFound = true; break;
					}
				}
				++typeIndex;
			}

			break;

		case MemoryTypes::hostVisible:
			typeIndex = 0;
			for ( auto const & type : memoryProperties.memoryTypes )
			{
				if ( type.propertyFlags & vk::MemoryPropertyFlagBits::eHostCoherent )
				{		
					if ( memoryProperties.memoryHeaps [ type.heapIndex ].size >= size )
					{
						typeFound = true; break;
					}
				}
				++typeIndex;
			}

			if ( typeFound ) break;

			typeIndex = 0;
			for ( auto const & type : memoryProperties.memoryTypes )
			{
				if ( type.propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible )
				{
					if ( memoryProperties.memoryHeaps [ type.heapIndex ].size >= size )
					{
						typeFound = true; break;
					}
				}
				++typeIndex;
			}

			break;
		}

		assert ( typeFound );

		return device.allocateMemory ( { size, typeIndex } );
	}

	void UpdateBuffer ( vk::PhysicalDevice physicalDevice, vk::Device device, vk::CommandPool commandPool, 
		vk::Queue queue, vk::Buffer buffer, void const * data, vk::DeviceSize size, vk::DeviceSize offset )
	{
		vk::Fence uploadFinishedFence { device.createFence ( {} ) };

		vk::Buffer stagingBuffer { CreateBuffer ( device, BufferUsages::stagingBuffer, size ) };
		vk::DeviceMemory stagingBufferMemory { AllocateMemory ( physicalDevice, device, MemoryTypes::hostVisible, size ) };
		device.bindBufferMemory ( stagingBuffer, stagingBufferMemory, 0 );

		auto stagingBufferData { device.mapMemory ( stagingBufferMemory, 0, size, {} ) };
		std::memcpy ( stagingBufferData, data, size );
		vk::MappedMemoryRange range { stagingBufferMemory, 0, size };
		device.flushMappedMemoryRanges ( { range } );
		device.unmapMemory ( stagingBufferMemory );

		auto commandBuffer { device.allocateCommandBuffers ( { commandPool, vk::CommandBufferLevel::ePrimary, 1 } ) [ 0 ] };

		vk::CommandBufferBeginInfo beginInfo {};
		commandBuffer.begin ( beginInfo );
		vk::BufferCopy copyRegion { 0, offset, size };
		commandBuffer.copyBuffer ( stagingBuffer, buffer, { copyRegion } );
		commandBuffer.end ();

		Submit ( queue, { commandBuffer }, uploadFinishedFence );
		device.waitForFences ( { uploadFinishedFence }, VK_FALSE, std::numeric_limits <uint64_t>::max () );

		device.destroy ( stagingBuffer );
		device.free ( stagingBufferMemory );
		device.destroy ( uploadFinishedFence );
		device.free ( commandPool, commandBuffer );
	}

	void CreateBuffer (
		vk::PhysicalDevice physicalDevice,
		vk::Device device,
		vk::CommandPool commandPool,
		vk::Queue queue,
		BufferUsages usage,
		void const * data,
		vk::DeviceSize size,
		vk::Buffer & buffer,
		vk::DeviceMemory & memory
	)
	{
		buffer = CreateBuffer ( device, usage, size );
		memory = AllocateMemory ( physicalDevice, device, MemoryTypes::deviceLocal, size );
		device.bindBufferMemory ( buffer, memory, 0 );
		UpdateBuffer ( physicalDevice, device, commandPool, queue, buffer, data, size );
	}

	void CreateBuffer (
		vk::PhysicalDevice physicalDevice,
		vk::Device device,
		BufferUsages usage,
		vk::DeviceSize size,
		vk::Buffer & buffer,
		vk::DeviceMemory & memory
	)
	{
		buffer = CreateBuffer ( device, usage, size );
		memory = AllocateMemory ( physicalDevice, device, MemoryTypes::deviceLocal, size );
		device.bindBufferMemory ( buffer, memory, 0 );
	}

	vk::DescriptorPool CreateDescriptorPool ( vk::Device device )
	{
		std::vector <vk::DescriptorPoolSize> poolSizes
		{
			{ vk::DescriptorType::eUniformBuffer, 1000 },
			{ vk::DescriptorType::eSampler, 1000 },
			{ vk::DescriptorType::eSampledImage, 1000 },
			{ vk::DescriptorType::eUniformBufferDynamic, 1000 }
		};

		vk::DescriptorPoolCreateInfo info { vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1000, poolSizes };
		return device.createDescriptorPool ( info );
	}

	vk::DescriptorSet AllocateDescriptorSet ( vk::Device device, vk::DescriptorPool pool, vk::DescriptorSetLayout layout )
	{
		auto layouts = { layout };
		return device.allocateDescriptorSets ( { pool, layouts } ) [ 0 ];
	}
	
	void CreateDepthBuffer ( vk::PhysicalDevice physicalDevice, vk::Device device, 
		vk::Extent2D extent, vk::Image & image, vk::DeviceMemory & memory, vk::ImageView & imageView )
	{
		vk::ImageCreateInfo imageCreateInfo
		{
			{},
			vk::ImageType::e2D,
			vk::Format::eD32Sfloat,
			{ extent.width, extent.height, 1 },
			1,
			1,
			vk::SampleCountFlagBits::e1,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eDepthStencilAttachment,
			vk::SharingMode::eExclusive,
			{},
			vk::ImageLayout::eUndefined
		};

		image = device.createImage ( imageCreateInfo );
		
		auto requirements { device.getImageMemoryRequirements ( image ) };
		memory = AllocateMemory ( physicalDevice, device, MemoryTypes::deviceLocal, requirements.size );
		device.bindImageMemory ( image, memory, 0 );

		vk::ImageViewCreateInfo imageViewCreateInfo
		{
			{},
			image,
			vk::ImageViewType::e2D,
			vk::Format::eD32Sfloat,
			{ vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity,vk::ComponentSwizzle::eIdentity,vk::ComponentSwizzle::eIdentity },
			{ vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1  }
		};

		imageView = device.createImageView ( imageViewCreateInfo );
	}

	vk::DescriptorSetLayout CreateDescriptorSetLayout ( 
		vk::Device device, 
		vk::DescriptorSetLayoutCreateFlags flags, 
		std::vector <vk::DescriptorSetLayoutBinding> const & bindings )
	{
		return device.createDescriptorSetLayout ( { {}, bindings } );
	}

	void CreateTexture (
		vk::PhysicalDevice physicalDevice,
		vk::Device device,
		vk::CommandPool commandPool,
		vk::Queue queue,
		uint32_t queueFamilyIndex,
		std::string const & filePath,
		vk::Image & image,
		vk::ImageView & imageView,
		vk::DeviceMemory & memory
	)
	{
		std::cout << "Creating texture: " << filePath << std::endl;

		assert ( std::filesystem::exists ( filePath ) );

		int width, height;
		stbi_set_flip_vertically_on_load ( 1 );
		auto data { stbi_load ( filePath.data (), &width, &height, nullptr, 4 ) };

		if ( ! data )
			std::cout << stbi_failure_reason () << std::endl;
		
		CreateTexture ( physicalDevice, device, commandPool, queue, queueFamilyIndex, data,
			{ static_cast < uint32_t > ( width ), static_cast < uint32_t > ( height ) }, 
			4, image, imageView, memory );
		
		std::cout << "Done" << std::endl;
	}
	
	void CreateTexture (
		vk::PhysicalDevice physicalDevice,
		vk::Device device,
		vk::CommandPool commandPool,
		vk::Queue queue,
		uint32_t queueFamilyIndex,
		unsigned char * data,
		vk::Extent2D extent,
		unsigned int components,
		vk::Image & image,
		vk::ImageView & imageView,
		vk::DeviceMemory & memory
	)
	{
		auto size { static_cast < vk::DeviceSize > ( extent.width * extent.height * components ) };

		auto format {
			components == 4 ? vk::Format::eR8G8B8A8Srgb
			: components == 3 ? vk::Format::eR8G8B8Srgb
			: components == 2 ? vk::Format::eR8G8Srgb
			: vk::Format::eR8Srgb

		};

		{
			vk::ImageCreateInfo createInfo
			{
				{},
				vk::ImageType::e2D,
				format,
				{ extent.width, extent.height, 1 },
				1,
				1,
				vk::SampleCountFlagBits::e1,
				vk::ImageTiling::eOptimal,
				vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
				{},
				{},
				vk::ImageLayout::eUndefined
			};

			image = device.createImage ( createInfo );
		}

		auto reqs { device.getImageMemoryRequirements ( image ) };
		memory = AllocateMemory ( physicalDevice, device, MemoryTypes::deviceLocal, reqs.size );
		device.bindImageMemory ( image, memory, 0 );

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

			imageView = device.createImageView ( createInfo );
		}

		vk::Fence uploadFinishedFence { device.createFence ( {} ) };

		vk::Buffer stagingBuffer { CreateBuffer ( device, BufferUsages::stagingBuffer, size ) };
		vk::DeviceMemory stagingBufferMemory { AllocateMemory ( physicalDevice, device, MemoryTypes::hostVisible, size ) };
		device.bindBufferMemory ( stagingBuffer, stagingBufferMemory, 0 );

		auto stagingBufferData { device.mapMemory ( stagingBufferMemory, 0, size, {} ) };
		std::memcpy ( stagingBufferData, data, size );
		vk::MappedMemoryRange range { stagingBufferMemory, 0, size };
		device.flushMappedMemoryRanges ( { range } );
		device.unmapMemory ( stagingBufferMemory );

		auto commandBuffer { device.allocateCommandBuffers ( { commandPool, vk::CommandBufferLevel::ePrimary, 1 } ) [ 0 ] };

		vk::CommandBufferBeginInfo beginInfo {};
		commandBuffer.begin ( beginInfo );

		// Transition layout to transfer dst optimal
		{
			vk::ImageMemoryBarrier imageMemoryBarrier (
				vk::AccessFlagBits::eNone, vk::AccessFlagBits::eTransferWrite,
				vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
				queueFamilyIndex, queueFamilyIndex,
				image, { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
			);

			auto imageMemoryBarriers = { imageMemoryBarrier };

			commandBuffer.pipelineBarrier ( vk::PipelineStageFlagBits::eAllCommands,
				vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlagBits::eByRegion, {}, {}, imageMemoryBarriers );
		}

		// Copy
		vk::BufferImageCopy copyRegion { 0, 0, 0, { vk::ImageAspectFlagBits::eColor, 0, 0, 1 }, {},
			{ extent.width, extent.height, 1 } };

		commandBuffer.copyBufferToImage ( stagingBuffer, image, vk::ImageLayout::eTransferDstOptimal, { copyRegion } );

		// Transition layout to shader read only optimal
		{
			vk::ImageMemoryBarrier imageMemoryBarrier (
				vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead,
				vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
				queueFamilyIndex, queueFamilyIndex,
				image,
				{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
			);

			auto imageMemoryBarriers = { imageMemoryBarrier };

			commandBuffer.pipelineBarrier ( vk::PipelineStageFlagBits::eTransfer,
				vk::PipelineStageFlagBits::eFragmentShader, vk::DependencyFlagBits::eByRegion, {}, {}, imageMemoryBarriers );
		}

		commandBuffer.end ();

		Submit ( queue, { commandBuffer }, uploadFinishedFence );
		device.waitForFences ( { uploadFinishedFence }, VK_FALSE, std::numeric_limits <uint64_t>::max () );

		device.destroy ( stagingBuffer );
		device.free ( stagingBufferMemory );
		device.destroy ( uploadFinishedFence );
		device.free ( commandPool, commandBuffer );
	}

	vk::Sampler CreateDefaultSampler ( vk::Device device )
	{
		vk::SamplerCreateInfo createInfo
		{
			{},
			vk::Filter::eLinear,
			vk::Filter::eLinear,
			vk::SamplerMipmapMode::eLinear,
			vk::SamplerAddressMode::eRepeat,
			vk::SamplerAddressMode::eRepeat,
			vk::SamplerAddressMode::eRepeat,
			0.0F,
			VK_FALSE,
			0.0f,
			VK_FALSE,
			vk::CompareOp::eNever,
			0.0f,
			0.0f,
			vk::BorderColor::eFloatOpaqueBlack,
			VK_FALSE
		};

		return device.createSampler ( createInfo );
	}

	void SetViewport ( vk::CommandBuffer commandBuffer, vk::Extent2D viewportExtent )
	{
		std::vector <vk::Viewport> viewports { {
			0,
			static_cast < float > ( viewportExtent.height ),
			static_cast < float > ( viewportExtent.width ),
			-static_cast < float > ( viewportExtent.height ),
			0.0f,
			1.0f
		} };

		std::vector <vk::Rect2D> scissors { { { 0, 0 }, viewportExtent } };

		commandBuffer.setViewport ( 0, viewports );
		commandBuffer.setScissor ( 0, scissors );
	}

	glm::mat4 CreateTransformMatrix (
		glm::vec3 const & translation,
		glm::vec3 const & scale
	)
	{
		glm::mat4 constexpr identityMat { glm::identity <glm::mat4> () };

		glm::mat4 translationMat { glm::translate ( identityMat, translation ) };
		glm::mat4 scaleMat { glm::scale ( identityMat, scale ) };

		return translationMat * scaleMat;
	}
}