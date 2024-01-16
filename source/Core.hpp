#pragma once

namespace pd
{
	std::vector < char const * > GetWindowRequiredVulkanExtensions ( SDL_Window * window );
	glm::vec2 GetWindowSize ( SDL_Window * window );
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
	vk::SwapchainKHR CreateSwapchain ( vk::PhysicalDevice, vk::Device, vk::SurfaceKHR, vk::SurfaceFormatKHR const &, glm::vec2 const & size );
	vk::RenderPass CreateRenderPass ( vk::Device, vk::Format outputFormat );
	std::vector <vk::ImageView> CreateSwapchainImageViews ( vk::Device, vk::SwapchainKHR, vk::Format format );
	std::vector <vk::Framebuffer> CreateFramebuffers ( vk::Device, vk::RenderPass, std::vector <vk::ImageView> attachments, glm::vec2 const & size );
}