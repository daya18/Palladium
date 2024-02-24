#pragma once

#include "Core.hpp"
#include "Axel.hpp"
#include "Recterer.hpp"
#include "Texterer.hpp"
#include "Camera.hpp"

#include <vk_mem_alloc.h>

namespace pd
{
	class Application
	{
	public:
		Application ();
		~Application ();

		void Run ();

	private:
		void HandleEvents ();
		void Update ();
		void Render ();
		void UpdateSwapchain ();
		
		bool quit { false };
		bool render { true };

		SDL_Window * window;
		vk::Instance instance;
		vk::DebugUtilsMessengerEXT debugUtilsMessenger;
		vk::SurfaceKHR surface;
		vk::PhysicalDevice physicalDevice;
		vk::Device device;
		DeviceQueues queues;
		VmaAllocator allocator;
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
		vk::CommandPool transferCommandPool;
		vk::CommandBuffer renderCommandBuffer;
		vk::Semaphore imageAvailableSemaphore;
		vk::Semaphore renderFinishedSemaphore;
		vk::Fence renderFinishedFence;

		Axel axel;
		Recterer recterer;
		Texterer texterer;

		float cameraMoveSensitivity { 0.01f };
		float cameraTurnSensitivity { 0.2f };
		bool dragging { false };
		glm::vec2 lastMousePosition {};
		glm::vec3 cameraMoveDirection {};
		Camera camera;
	};
}