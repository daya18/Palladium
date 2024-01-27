#pragma once

#include "Core.hpp"
#include "Axel.hpp"
#include "Camera.hpp"

namespace pd
{
	class Application
	{
	public:
		Application ();
		~Application ();

		void Run ();

	private:
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
		vk::Image depthBuffer;
		vk::DeviceMemory depthBufferMemory;
		vk::ImageView depthBufferView;
		std::vector <vk::Framebuffer> framebuffers;
		vk::CommandPool graphicsCommandPool;
		vk::CommandBuffer renderCommandBuffer;
		vk::Semaphore imageAvailableSemaphore;
		vk::Semaphore renderFinishedSemaphore;
		vk::Fence renderFinishedFence;

		Axel axel;
		Camera camera;
	};
}