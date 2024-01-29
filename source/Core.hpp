#pragma once

namespace pd
{
	std::vector < char const * > GetWindowRequiredVulkanExtensions ( SDL_Window * window );
	glm::vec2 GetWindowSize ( SDL_Window * window );
	glm::vec2 GetMousePosition ();

	vk::Instance CreateInstance ( SDL_Window * window );
	vk::SurfaceKHR CreateWindowSurface ( vk::Instance, SDL_Window * window );

	VkBool32 debugCallback (
		VkDebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT              messageTypes,
		const VkDebugUtilsMessengerCallbackDataEXT * pCallbackData,
		void *
	);
	
	vk::DebugUtilsMessengerEXT CreateDebugUtilsMessenger ( vk::Instance );
	vk::PhysicalDevice SelectPhysicalDevice ( vk::Instance, vk::SurfaceKHR );

	struct DeviceQueues
	{
		uint32_t graphicsQueueFamilyIndex;
		uint32_t presentationQueueFamilyIndex;
		uint32_t transferQueueFamilyIndex;

		vk::Queue graphicsQueue;
		vk::Queue presentationQueue;
		vk::Queue transferQueue;
	};

	void CreateDevice ( vk::PhysicalDevice, vk::SurfaceKHR surface, vk::Device &, DeviceQueues & );
	vk::SurfaceFormatKHR SelectSurfaceFormat ( vk::PhysicalDevice, vk::SurfaceKHR );
	vk::SwapchainKHR CreateSwapchain ( vk::PhysicalDevice, vk::Device, vk::SurfaceKHR, vk::SurfaceFormatKHR const &, glm::vec2 const & size, vk::SwapchainKHR oldSwapchain = {} );
	vk::RenderPass CreateRenderPass ( vk::Device, vk::Format outputFormat );
	std::vector <vk::ImageView> CreateSwapchainImageViews ( vk::Device, vk::SwapchainKHR, vk::Format format );
	std::vector <vk::Framebuffer> CreateFramebuffers ( vk::Device, vk::RenderPass, std::vector <vk::ImageView> attachments, vk::ImageView depthAttachment, glm::vec2 const & size );
	vk::PipelineLayout CreatePipelineLayout ( vk::Device, std::vector <vk::DescriptorSetLayout> const & = {} );
	vk::ShaderModule CreateShaderModuleFromFile ( vk::Device, std::string const & filePath );

	struct GraphicsPipelineCreateInfo
	{
		vk::Device device; 
		vk::RenderPass renderPass;
		uint32_t subpass;
		vk::PipelineLayout pipelineLayout;
	};

	vk::Pipeline CreateGraphicsPipeline ( GraphicsPipelineCreateInfo const & );
	
	void Submit ( 
		vk::Queue, 
		std::vector <vk::CommandBuffer> const & commandBuffers,
		vk::Fence signalFence = {},
		std::vector <vk::Semaphore> signalSemaphores = {},
		std::vector <vk::Semaphore> const & waitSemaphores = {},
		std::vector <vk::PipelineStageFlags> waitStages = {}
	);

	vk::Result Present ( vk::Queue, vk::SwapchainKHR, uint32_t imageIndex, vk::Semaphore waitSemaphore );
	enum class BufferUsages { vertexBuffer, indexBuffer, uniformBuffer, stagingBuffer };
	vk::Buffer CreateBuffer ( vk::Device, BufferUsages, vk::DeviceSize size );
	enum class MemoryTypes { hostVisible, deviceLocal };
	vk::DeviceMemory AllocateMemory ( vk::PhysicalDevice, vk::Device, MemoryTypes, vk::DeviceSize size );
	
	void UpdateBuffer ( vk::PhysicalDevice physicalDevice, vk::Device device, vk::CommandPool commandPool,
		vk::Queue queue, vk::Buffer buffer, void const * data, vk::DeviceSize size );

	void CreateBuffer ( 
		vk::PhysicalDevice,
		vk::Device,
		vk::CommandPool,
		vk::Queue,
		BufferUsages usage,
		void const * data,
		vk::DeviceSize size,
		vk::Buffer &, 
		vk::DeviceMemory & 
	);

	vk::DescriptorPool CreateDescriptorPool ( vk::Device );
	vk::DescriptorSet AllocateDescriptorSet ( vk::Device, vk::DescriptorPool, vk::DescriptorSetLayout );
	void CreateDepthBuffer ( vk::PhysicalDevice, vk::Device, vk::Extent2D, vk::Image &, vk::DeviceMemory &, vk::ImageView & );
	vk::DescriptorSetLayout CreateDescriptorSetLayout ( vk::Device, vk::DescriptorSetLayoutCreateFlags, std::vector <vk::DescriptorSetLayoutBinding> const & );
}