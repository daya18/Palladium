#include "Application.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

namespace pd
{
	Application::Application ()
	{
		SDL_Init ( 0 );
		window = SDL_CreateWindow ( "Palladium", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_VULKAN );
		
		VULKAN_HPP_DEFAULT_DISPATCHER.init ();
		instance = CreateInstance ( window );
		VULKAN_HPP_DEFAULT_DISPATCHER.init ( instance );
		debugUtilsMessenger = CreateDebugUtilsMessenger ( instance );
		surface = CreateWindowSurface ( instance, window );
		physicalDevice = SelectPhysicalDevice ( instance, surface );
		CreateDevice ( physicalDevice, surface, device, queues );
		VULKAN_HPP_DEFAULT_DISPATCHER.init ( device );
		vk::SurfaceFormatKHR surfaceFormat { SelectSurfaceFormat ( physicalDevice, surface ) };
		auto windowSize { GetWindowSize ( window ) };
		swapchainExtent = vk::Extent2D { static_cast < uint32_t > ( windowSize.x ), static_cast < uint32_t > ( windowSize.y ) };
		swapchain = CreateSwapchain ( physicalDevice, device, surface, surfaceFormat, windowSize );
		renderPass = CreateRenderPass ( device, surfaceFormat.format );
		swapchainImageViews = CreateSwapchainImageViews ( device, swapchain, surfaceFormat.format );
		framebuffers = CreateFramebuffers ( device, renderPass, swapchainImageViews, windowSize );
		graphicsCommandPool = device.createCommandPool ( { { vk::CommandPoolCreateFlagBits::eResetCommandBuffer }, queues.graphicsQueueFamilyIndex } );
		renderCommandBuffer = device.allocateCommandBuffers ( { graphicsCommandPool, vk::CommandBufferLevel::ePrimary, 1 } ) [ 0 ];
		imageAvailableSemaphore = device.createSemaphore ( {} );
		renderFinishedSemaphore = device.createSemaphore ( {} );
		renderFinishedFence = device.createFence ( { vk::FenceCreateFlagBits::eSignaled } );
	}

	Application::~Application ()
	{
		device.waitIdle ();

		device.destroy ( renderFinishedFence );
		device.destroy ( renderFinishedSemaphore );
		device.destroy ( imageAvailableSemaphore );

		device.freeCommandBuffers ( graphicsCommandPool, { renderCommandBuffer } );

		device.destroy ( graphicsCommandPool );

		for ( auto const & framebuffer : framebuffers )
			device.destroy ( framebuffer );

		for ( auto const & imageView : swapchainImageViews )
			device.destroy ( imageView );

		device.destroy ( renderPass );
		device.destroy ( swapchain );
		device.destroy ();
		instance.destroy ( surface );
		instance.destroy ( debugUtilsMessenger );
		instance.destroy ();

		SDL_DestroyWindow ( window );
		SDL_Quit ();
	}

	void Application::Run ()
	{
		bool quit { false };

		while ( ! quit )
		{
			// Process events
			SDL_Event event;
			while ( SDL_PollEvent ( &event ) )
			{
				switch ( event.type )
				{
				case SDL_WINDOWEVENT:
					switch ( event.window.event )
					{
					case SDL_WINDOWEVENT_CLOSE:
						quit = true;
						break;
					}
					break;
				}
			}

			// Render
			device.waitForFences ( { renderFinishedFence }, VK_FALSE, std::numeric_limits <uint64_t>::max () );
			device.resetFences ( { renderFinishedFence } );

			auto acquireResult { device.acquireNextImageKHR ( swapchain, std::numeric_limits <uint64_t>::max (), imageAvailableSemaphore, {} ) };
			auto imageIndex { acquireResult.value };

				// Record
			vk::CommandBufferInheritanceInfo inheritanceInfo { renderPass, 0, framebuffers [ imageIndex ] };
			vk::CommandBufferBeginInfo beginInfo { {}, &inheritanceInfo };

			renderCommandBuffer.begin ( beginInfo );

			vk::Rect2D renderArea { { 0, 0 }, swapchainExtent };
			std::vector <vk::ClearValue> clearValues { { { 0.0f, 1.0f, 0.0f, 0.0f } } };
			vk::RenderPassBeginInfo renderPassBeginInfo { renderPass, framebuffers [ imageIndex ], renderArea, clearValues };
			
			renderCommandBuffer.beginRenderPass ( renderPassBeginInfo, vk::SubpassContents::eInline );

			renderCommandBuffer.endRenderPass ();

			renderCommandBuffer.end ();

				// Submit
			
			{
				auto waitSemaphores = { imageAvailableSemaphore };
				std::vector <vk::PipelineStageFlags> waitStages = { vk::PipelineStageFlagBits::eTopOfPipe };
				auto commandBuffers = { renderCommandBuffer };
				auto signalSemaphores = { renderFinishedSemaphore };

				vk::SubmitInfo info
				{
					waitSemaphores,
					waitStages,
					renderCommandBuffer,
					signalSemaphores
				};

				queues.graphicsQueue.submit ( { info }, renderFinishedFence );
			}

			{
				auto waitSemaphores = { renderFinishedSemaphore };
				auto swapchains = { swapchain };
				std::vector <uint32_t> imageIndices { imageIndex };

				vk::PresentInfoKHR presentInfo
				{
					waitSemaphores,
					swapchains,
					imageIndices
				};

				queues.presentationQueue.presentKHR ( presentInfo );
			}
		}
	}
}