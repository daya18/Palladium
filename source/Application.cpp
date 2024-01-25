#include "Application.hpp"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

namespace pd
{
	Application::Application ()
	{
		SDL_Init ( 0 );
		window = SDL_CreateWindow ( "Palladium", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE );

		VULKAN_HPP_DEFAULT_DISPATCHER.init ();
		instance = CreateInstance ( window );
		VULKAN_HPP_DEFAULT_DISPATCHER.init ( instance );
		debugUtilsMessenger = CreateDebugUtilsMessenger ( instance );
		surface = CreateWindowSurface ( instance, window );
		physicalDevice = SelectPhysicalDevice ( instance, surface );
		surfaceFormat = SelectSurfaceFormat ( physicalDevice, surface );
		CreateDevice ( physicalDevice, surface, device, queues );
		VULKAN_HPP_DEFAULT_DISPATCHER.init ( device );
		auto windowSize { GetWindowSize ( window ) };
		swapchain = CreateSwapchain ( physicalDevice, device, surface, surfaceFormat, windowSize );
		swapchainExtent = vk::Extent2D { static_cast < uint32_t > ( windowSize.x ), static_cast < uint32_t > ( windowSize.y ) };
		renderPass = CreateRenderPass ( device, surfaceFormat.format );
		swapchainImageViews = CreateSwapchainImageViews ( device, swapchain, surfaceFormat.format );
		framebuffers = CreateFramebuffers ( device, renderPass, swapchainImageViews, windowSize );
		graphicsCommandPool = device.createCommandPool ( { { vk::CommandPoolCreateFlagBits::eResetCommandBuffer }, queues.graphicsQueueFamilyIndex } );
		renderCommandBuffer = device.allocateCommandBuffers ( { graphicsCommandPool, vk::CommandBufferLevel::ePrimary, 1 } ) [ 0 ];
		imageAvailableSemaphore = device.createSemaphore ( {} );
		renderFinishedSemaphore = device.createSemaphore ( {} );
		renderFinishedFence = device.createFence ( { vk::FenceCreateFlagBits::eSignaled } );
		std::vector <vk::DescriptorSetLayoutBinding> bindings { { 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment } };
		materialDSetLayout = device.createDescriptorSetLayout ( { {}, bindings } );
		pipelineLayout = CreatePipelineLayout ( device, { materialDSetLayout } );
		graphicsPipeline = CreateGraphicsPipeline ( { device, renderPass, 0, pipelineLayout } );
		transferCommandPool = device.createCommandPool ( { {}, queues.transferQueueFamilyIndex } );
		descriptorPool = CreateDescriptorPool ( device );

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

		MaterialUniformBlock materialData { { 0.0f, 1.0f, 0.0f, 1.0f } };
		
		CreateBuffer ( physicalDevice, device, transferCommandPool, queues.transferQueue, 
			BufferUsages::vertexBuffer, vertices.data (), vertices.size () * sizeof ( float ), vertexBuffer, vertexBufferMemory );

		CreateBuffer ( physicalDevice, device, transferCommandPool, queues.transferQueue,
			BufferUsages::indexBuffer, indices.data (), indices.size () * sizeof ( uint32_t ), indexBuffer, indexBufferMemory );
		
		CreateBuffer ( physicalDevice, device, transferCommandPool, queues.transferQueue,
			BufferUsages::uniformBuffer, &materialData, sizeof ( MaterialUniformBlock ), materialUniformBuffer, materialUniformBufferMemory );

		materialDescriptorSet = AllocateDescriptorSet ( device, descriptorPool, materialDSetLayout );

		vk::DescriptorBufferInfo bufferInfo { materialUniformBuffer, 0, sizeof ( MaterialUniformBlock ) };
		vk::WriteDescriptorSet write { materialDescriptorSet, 0, 0, 1, vk::DescriptorType::eUniformBuffer, {}, &bufferInfo };
		device.updateDescriptorSets ( { write }, {} );
	}

	Application::~Application ()
	{
		device.waitIdle ();
		device.destroy ( materialUniformBuffer );
		device.free ( materialUniformBufferMemory );
		device.free ( indexBufferMemory );
		device.destroy ( indexBuffer );
		device.free ( vertexBufferMemory );
		device.destroy ( vertexBuffer );
		device.destroy ( descriptorPool );
		device.destroy ( transferCommandPool );
		device.destroy ( graphicsPipeline );
		device.destroy ( pipelineLayout );
		device.destroy ( materialDSetLayout );
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

			Render ();
			
		}
	}
	
	void Application::Render ()
	{
		device.waitForFences ( { renderFinishedFence }, VK_FALSE, std::numeric_limits <uint64_t>::max () );
		device.resetFences ( { renderFinishedFence } );

		auto acquireResult { device.acquireNextImageKHR ( swapchain, std::numeric_limits <uint64_t>::max (), imageAvailableSemaphore, {} ) };

		if ( acquireResult.result == vk::Result::eSuboptimalKHR || acquireResult.result == vk::Result::eErrorOutOfDateKHR )
		{
			UpdateSwapchain ();
			return;
		}


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

		std::vector <vk::Viewport> viewports { { 0, 0, static_cast < float > ( swapchainExtent.width ),
			static_cast < float > ( swapchainExtent.height ), 0.0f, 1.0f } };

		std::vector <vk::Rect2D> scissors { { { 0, 0 }, swapchainExtent } };

		renderCommandBuffer.setViewport ( 0, viewports );
		renderCommandBuffer.setScissor ( 0, scissors );

		renderCommandBuffer.bindDescriptorSets ( vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, { materialDescriptorSet }, {} );
		renderCommandBuffer.bindVertexBuffers ( 0, { vertexBuffer }, { 0 } );
		renderCommandBuffer.bindIndexBuffer ( indexBuffer, 0, vk::IndexType::eUint32 );
		renderCommandBuffer.drawIndexed ( indexCount, 1, 0, 0, 0 );

		renderCommandBuffer.endRenderPass ();

		renderCommandBuffer.end ();

		Submit ( queues.graphicsQueue, { renderCommandBuffer }, renderFinishedFence, { renderFinishedSemaphore },
			{ imageAvailableSemaphore }, { vk::PipelineStageFlagBits::eTopOfPipe } );

		auto presentResult { Present ( queues.presentationQueue, swapchain, imageIndex, renderFinishedSemaphore ) };

		if ( presentResult == vk::Result::eSuboptimalKHR || presentResult == vk::Result::eErrorOutOfDateKHR )
		{
			device.waitIdle ();
			UpdateSwapchain ();
			return;
		}
	}

	void Application::UpdateSwapchain ()
	{
		auto windowSize { GetWindowSize ( window ) };
		auto oldSwapchain { swapchain };
		swapchain = CreateSwapchain ( physicalDevice, device, surface, surfaceFormat, windowSize, oldSwapchain );
		device.destroy ( oldSwapchain );
		swapchainExtent = vk::Extent2D { static_cast < uint32_t > ( windowSize.x ), static_cast < uint32_t > ( windowSize.y ) };

		device.destroy ( renderPass );
		renderPass = CreateRenderPass ( device, surfaceFormat.format );
		
		for ( auto const & imageView : swapchainImageViews )
			device.destroy ( imageView );
		swapchainImageViews = CreateSwapchainImageViews ( device, swapchain, surfaceFormat.format );

		for ( auto const & framebuffer : framebuffers )
			device.destroy ( framebuffer );
		framebuffers = CreateFramebuffers ( device, renderPass, swapchainImageViews, windowSize );
		
		device.free ( graphicsCommandPool, renderCommandBuffer );
		renderCommandBuffer = device.allocateCommandBuffers ( { graphicsCommandPool, vk::CommandBufferLevel::ePrimary, 1 } ) [ 0 ];

		device.destroy ( imageAvailableSemaphore );
		imageAvailableSemaphore = device.createSemaphore ( {} );

		device.destroy ( renderFinishedSemaphore );
		renderFinishedSemaphore = device.createSemaphore ( {} );

		device.destroy ( renderFinishedFence );
		renderFinishedFence = device.createFence ( { vk::FenceCreateFlagBits::eSignaled } );
	}

}