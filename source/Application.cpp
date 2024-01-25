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
		pipelineLayout = CreatePipelineLayout ( device );
		graphicsPipeline = CreateGraphicsPipeline ( { device, renderPass, 0, pipelineLayout } );
		transferCommandPool = device.createCommandPool ( { {}, queues.transferQueueFamilyIndex } );

		std::vector <float> vertices {
			-0.5f,  0.5f, 0.0f,
			-0.5f, -0.5f, 0.0f,
			 0.5f, -0.5f, 0.0f,
			 0.5f,  0.5f, 0.0f
		};

		std::vector <uint32_t> indices {
			0, 1, 2, 2, 3, 0
		};

		indexCount = indices.size ();

		CreateBuffer ( physicalDevice, device, transferCommandPool, queues.transferQueue, 
			BufferUsages::vertexBuffer, vertices.data (), vertices.size () * sizeof ( float ), vertexBuffer, vertexBufferMemory );

		CreateBuffer ( physicalDevice, device, transferCommandPool, queues.transferQueue,
			BufferUsages::indexBuffer, indices.data (), indices.size () * sizeof ( uint32_t ), indexBuffer, indexBufferMemory );
	}

	Application::~Application ()
	{
		device.waitIdle ();
		
		device.free ( indexBufferMemory );
		device.destroy ( indexBuffer );
		device.free ( vertexBufferMemory );
		device.destroy ( vertexBuffer );
		device.destroy ( transferCommandPool );
		device.destroy ( graphicsPipeline );
		device.destroy ( pipelineLayout );
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
			std::vector <vk::ClearValue> clearValues { { { 0.0f, 0.0f, 0.0f, 1.0f } } };
			vk::RenderPassBeginInfo renderPassBeginInfo { renderPass, framebuffers [ imageIndex ], renderArea, clearValues };
			
			renderCommandBuffer.beginRenderPass ( renderPassBeginInfo, vk::SubpassContents::eInline );

			renderCommandBuffer.bindPipeline ( vk::PipelineBindPoint::eGraphics, graphicsPipeline );
			renderCommandBuffer.bindVertexBuffers ( 0, { vertexBuffer }, { 0 } );
			renderCommandBuffer.bindIndexBuffer ( indexBuffer, 0, vk::IndexType::eUint32 );
			renderCommandBuffer.drawIndexed ( indexCount, 1, 0, 0, 0 );
			
			renderCommandBuffer.endRenderPass ();

			renderCommandBuffer.end ();

			Submit ( queues.graphicsQueue, { renderCommandBuffer }, renderFinishedFence, { renderFinishedSemaphore },
				{ imageAvailableSemaphore }, { vk::PipelineStageFlagBits::eTopOfPipe } );

			Present ( queues.presentationQueue, swapchain, imageIndex, renderFinishedSemaphore );
		}
	}
}