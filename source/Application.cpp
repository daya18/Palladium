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
		CreateDepthBuffer ( physicalDevice, device, swapchainExtent, depthBuffer, depthBufferMemory, depthBufferView );
		framebuffers = CreateFramebuffers ( device, renderPass, swapchainImageViews, depthBufferView, windowSize );
		graphicsCommandPool = device.createCommandPool ( { { vk::CommandPoolCreateFlagBits::eResetCommandBuffer }, queues.graphicsQueueFamilyIndex } );
		renderCommandBuffer = device.allocateCommandBuffers ( { graphicsCommandPool, vk::CommandBufferLevel::ePrimary, 1 } ) [ 0 ];
		imageAvailableSemaphore = device.createSemaphore ( {} );
		renderFinishedSemaphore = device.createSemaphore ( {} );
		renderFinishedFence = device.createFence ( { vk::FenceCreateFlagBits::eSignaled } );

		camera.SetViewportSize ( windowSize );
		camera.SetPosition ( { 0.0f, 0.0f, 1.0f } );

		axel.Initialize ( { physicalDevice, device, &queues, renderPass } );
		axel.LoadScene ( "scene/TestScene.obj" );
		axel.SetCamera ( camera );
	}

	Application::~Application ()
	{
		device.waitIdle ();

		axel.Shutdown ();

		device.destroy ( renderFinishedFence );
		device.destroy ( renderFinishedSemaphore );
		device.destroy ( imageAvailableSemaphore );

		device.freeCommandBuffers ( graphicsCommandPool, { renderCommandBuffer } );

		device.destroy ( graphicsCommandPool );

		for ( auto const & framebuffer : framebuffers )
			device.destroy ( framebuffer );

		device.destroy ( depthBufferView );
		device.destroy ( depthBuffer );
		device.free ( depthBufferMemory );

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
		while ( ! quit )
		{
			HandleEvents ();

			Update ();

			if ( render )
				Render ();

		}
	}

	void Application::HandleEvents ()
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

				case SDL_WINDOWEVENT_RESIZED:
				case SDL_WINDOWEVENT_SIZE_CHANGED:
					auto windowSize { GetWindowSize ( window ) };
					camera.SetViewportSize ( windowSize );
					axel.SetCamera ( camera );
					break;

				case SDL_WINDOWEVENT_MINIMIZED:
					render = false;
					break;

				case SDL_WINDOWEVENT_RESTORED:
					render = true;
					break;
				}
				break;

			case SDL_MOUSEBUTTONDOWN:
				lastMousePosition = GetMousePosition ();
				dragging = true;
				break;

			case SDL_MOUSEBUTTONUP:
				dragging = false;
				break;

			case SDL_KEYDOWN:
				if ( ! event.key.repeat )
				{
					switch ( event.key.keysym.scancode )
					{
					case SDL_SCANCODE_A: cameraMoveDirection.x += -1.0f; break;
					case SDL_SCANCODE_D: cameraMoveDirection.x += 1.0f; break;
					case SDL_SCANCODE_W: cameraMoveDirection.z += 1.0f; break;
					case SDL_SCANCODE_S: cameraMoveDirection.z += -1.0f; break;
					case SDL_SCANCODE_SPACE: cameraMoveDirection.y += 1.0f; break;
					case SDL_SCANCODE_LSHIFT: cameraMoveDirection.y += -1.0f; break;
					}
					break;
				}

			case SDL_KEYUP:
				if ( ! event.key.repeat )
				{
					switch ( event.key.keysym.scancode )
					{
					case SDL_SCANCODE_A: cameraMoveDirection.x += 1.0f; break;
					case SDL_SCANCODE_D: cameraMoveDirection.x += -1.0f; break;
					case SDL_SCANCODE_W: cameraMoveDirection.z += -1.0f; break;
					case SDL_SCANCODE_S: cameraMoveDirection.z += 1.0f; break;
					case SDL_SCANCODE_SPACE: cameraMoveDirection.y += -1.0f; break;
					case SDL_SCANCODE_LSHIFT: cameraMoveDirection.y += 1.0f; break;
					}
					break;
				}
				
				//case SDL_EventType::SDL_MOUSEMOTION:
				//	int x, y;
				//	SDL_GetMouseState ( &x, &y );
				//	std::cout << x << ' ' << y << std::endl;
				//	break;
			}
		}
	}

	void Application::Update ()
	{
		camera.Move ( cameraMoveDirection * cameraMoveSensitivity );

		if ( dragging )
		{
			auto currentMousePosition { GetMousePosition () };
			auto mouseDelta { currentMousePosition - lastMousePosition };
			lastMousePosition = currentMousePosition;
			
			auto cameraTurnDelta { mouseDelta };
			cameraTurnDelta *= cameraTurnSensitivity;

			camera.PitchYaw ( cameraTurnDelta.y, cameraTurnDelta.x );
		}

		axel.SetCamera ( camera );
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
		std::vector <vk::ClearValue> clearValues { { { 0.0f, 0.0f, 0.0f, 1.0f } }, { { 1.0f } } };
		vk::RenderPassBeginInfo renderPassBeginInfo { renderPass, framebuffers [ imageIndex ], renderArea, clearValues };

		renderCommandBuffer.beginRenderPass ( renderPassBeginInfo, vk::SubpassContents::eInline );
		
		axel.RecordRender ( renderCommandBuffer, swapchainExtent );

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
		int windowWidth, windowHeight;
		SDL_Vulkan_GetDrawableSize ( window, &windowWidth, &windowHeight );
		std::cout << windowHeight << std::endl;

		// minimize/maximize detection
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
		
		device.destroy ( depthBufferView );
		device.destroy ( depthBuffer );
		device.free ( depthBufferMemory );
		
		CreateDepthBuffer ( physicalDevice, device, swapchainExtent, depthBuffer, depthBufferMemory, depthBufferView );
		
		framebuffers = CreateFramebuffers ( device, renderPass, swapchainImageViews, depthBufferView, windowSize );
		
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