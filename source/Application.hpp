#pragma once

#include "Core.hpp"

namespace pd
{
	class Application
	{
	public:
		Application ();
		~Application ();

		void Run ();

	private:
		struct MaterialUniformBlock
		{
			glm::vec4 color;
		};

		void Render ();
		void UpdateSwapchain ();

		SDL_Window * window;
		vk::Instance instance;
		vk::DebugUtilsMessengerEXT debugUtilsMessenger;
		vk::SurfaceKHR surface;
		vk::PhysicalDevice physicalDevice;
		vk::Device device;
		DeviceQueues queues;
		vk::SurfaceFormatKHR surfaceFormat;
		vk::SwapchainKHR swapchain;
		vk::Extent2D swapchainExtent;
		vk::RenderPass renderPass;
		std::vector <vk::ImageView> swapchainImageViews;
		std::vector <vk::Framebuffer> framebuffers;
		vk::CommandPool graphicsCommandPool;
		vk::CommandBuffer renderCommandBuffer;
		vk::Semaphore imageAvailableSemaphore;
		vk::Semaphore renderFinishedSemaphore;
		vk::Fence renderFinishedFence;
		vk::DescriptorSetLayout materialDSetLayout;
		vk::PipelineLayout pipelineLayout;
		vk::Pipeline graphicsPipeline;
		vk::CommandPool transferCommandPool;
		vk::DescriptorPool descriptorPool;

		vk::Buffer vertexBuffer;
		vk::DeviceMemory vertexBufferMemory;
		uint32_t indexCount { 0 };
		vk::Buffer indexBuffer;
		vk::DeviceMemory indexBufferMemory;
		vk::Buffer materialUniformBuffer;
		vk::DeviceMemory materialUniformBufferMemory;
		vk::DescriptorSet materialDescriptorSet;
	};
}