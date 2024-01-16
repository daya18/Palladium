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
		SDL_Window * window;
		vk::Instance instance;
		vk::DebugUtilsMessengerEXT debugUtilsMessenger;
		vk::SurfaceKHR surface;
		vk::PhysicalDevice physicalDevice;
		vk::Device device;
		DeviceQueues queues;
		vk::Extent2D swapchainExtent;
		vk::SwapchainKHR swapchain;
		vk::RenderPass renderPass;
		std::vector <vk::ImageView> swapchainImageViews;
		std::vector <vk::Framebuffer> framebuffers;
		vk::CommandPool graphicsCommandPool;
		vk::CommandBuffer renderCommandBuffer;
		vk::Semaphore imageAvailableSemaphore;
		vk::Semaphore renderFinishedSemaphore;
		vk::Fence renderFinishedFence;
	};
}