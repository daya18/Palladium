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

	vk::PipelineLayout CreatePipelineLayout ( vk::Device device, std::vector <vk::DescriptorSetLayout> const & descriptorSetLayouts )
	{
		vk::PipelineLayoutCreateInfo createInfo
		{
			{},
			descriptorSetLayouts,
			{}
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

		std::vector <vk::VertexInputBindingDescription> vertexBindings { { 0, sizeof ( float ) * 3, vk::VertexInputRate::eVertex } };
		std::vector <vk::VertexInputAttributeDescription> vertexAttributes { { 0, 0, vk::Format::eR32G32B32Sfloat, 0 } };

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
					typeFound = true; break;
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
					typeFound = true; break;
				}
				++typeIndex;
			}

			if ( typeFound ) break;

			typeIndex = 0;
			for ( auto const & type : memoryProperties.memoryTypes )
			{
				if ( type.propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible )
				{
					typeFound = true; break;
				}
				++typeIndex;
			}

			break;
		}

		assert ( typeFound );

		return device.allocateMemory ( { size, typeIndex } );
	}

	void UpdateBuffer ( vk::PhysicalDevice physicalDevice, vk::Device device, vk::CommandPool commandPool, 
		vk::Queue queue, vk::Buffer buffer, void const * data, vk::DeviceSize size )
	{
		vk::Fence uploadFinishedFence { device.createFence ( {} ) };

		vk::Buffer stagingBuffer { CreateBuffer ( device, BufferUsages::stagingBuffer, size ) };
		vk::DeviceMemory stagingBufferMemory { AllocateMemory ( physicalDevice, device, MemoryTypes::hostVisible, size ) };
		device.bindBufferMemory ( stagingBuffer, stagingBufferMemory, 0 );

		auto stagingBufferData { device.mapMemory ( stagingBufferMemory, 0, size, {} ) };
		std::memcpy ( stagingBufferData, data, size );
		vk::MappedMemoryRange range { stagingBufferMemory, 0, size };
		device.flushMappedMemoryRanges ( { range } );

		auto commandBuffer { device.allocateCommandBuffers ( { commandPool, vk::CommandBufferLevel::ePrimary, 1 } ) [ 0 ] };

		vk::CommandBufferBeginInfo beginInfo {};
		commandBuffer.begin ( beginInfo );
		vk::BufferCopy copyRegion { 0, 0, size };
		commandBuffer.copyBuffer ( stagingBuffer, buffer, { copyRegion } );
		commandBuffer.end ();

		Submit ( queue, { commandBuffer }, uploadFinishedFence );
		device.waitForFences ( { uploadFinishedFence }, VK_FALSE, std::numeric_limits <uint64_t>::max () );

		device.destroy ( stagingBuffer );
		device.free ( stagingBufferMemory );
		device.destroy ( uploadFinishedFence );
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

	vk::DescriptorPool CreateDescriptorPool ( vk::Device device )
	{
		std::vector <vk::DescriptorPoolSize> poolSizes
		{
			{ vk::DescriptorType::eUniformBuffer, 1000 },
			{ vk::DescriptorType::eSampledImage, 1000 }
		};

		vk::DescriptorPoolCreateInfo info { {}, 1000, poolSizes };
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
}