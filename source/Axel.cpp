#include "Axel.hpp"

#include "OBJ_Loader.h"

namespace pd
{
	void Axel::Initialize ( Dependencies const & deps )
	{
		this->deps = deps;

		{
			vk::SamplerCreateInfo createInfo
			{
				{},
				vk::Filter::eLinear,
				vk::Filter::eLinear,
				vk::SamplerMipmapMode::eLinear,
				vk::SamplerAddressMode::eRepeat,
				vk::SamplerAddressMode::eRepeat,
				vk::SamplerAddressMode::eRepeat,
				0.0F,
				VK_FALSE,
				0.0f,
				VK_FALSE,
				vk::CompareOp::eNever,
				0.0f,
				0.0f,
				vk::BorderColor::eFloatOpaqueWhite,
				VK_FALSE
			};
			
			sampler = deps.device.createSampler ( createInfo );
		}

		materialDSetLayout = CreateDescriptorSetLayout ( deps.device, {}, {
			{ 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment },
			{ 1, vk::DescriptorType::eSampler, 1, vk::ShaderStageFlagBits::eFragment, &sampler },
			{ 2, vk::DescriptorType::eSampledImage, 1, vk::ShaderStageFlagBits::eFragment }
		} );

		cameraDSetLayout = CreateDescriptorSetLayout ( deps.device, {}, { { 0, vk::DescriptorType::eUniformBuffer, 1,
			vk::ShaderStageFlagBits::eVertex } } );

		pipelineLayout = CreatePipelineLayout ( deps.device, { materialDSetLayout, cameraDSetLayout } );
		graphicsPipeline = CreateGraphicsPipeline ( { deps.device, deps.renderPass, 0, pipelineLayout } );
		transferCommandPool = deps.device.createCommandPool ( { {}, deps.queues->transferQueueFamilyIndex } );
		descriptorPool = CreateDescriptorPool ( deps.device );

		CreateTexture ( deps.physicalDevice, deps.device, transferCommandPool, 
			deps.queues->transferQueue, deps.queues->transferQueueFamilyIndex,
			"image/TestImage.jpg", texImage, texImageView, texMemory );
		
		MaterialUniformBlock materialData { { 0.0f, 1.0f, 0.0f, 1.0f } };

		{
			CreateBuffer ( deps.physicalDevice, deps.device, transferCommandPool, deps.queues->transferQueue,
				BufferUsages::uniformBuffer, &materialData, sizeof ( MaterialUniformBlock ), materialUniformBuffer, materialUniformBufferMemory );

			materialDescriptorSet = AllocateDescriptorSet ( deps.device, descriptorPool, materialDSetLayout );

			vk::DescriptorBufferInfo bufferInfo { materialUniformBuffer, 0, sizeof ( MaterialUniformBlock ) };
			vk::DescriptorImageInfo imageInfo { {}, texImageView, vk::ImageLayout::eShaderReadOnlyOptimal };
			
			std::vector <vk::WriteDescriptorSet> writes { 
				{ materialDescriptorSet, 0, 0, 1, vk::DescriptorType::eUniformBuffer, {}, &bufferInfo },
				{ materialDescriptorSet, 2, 0, 1, vk::DescriptorType::eSampledImage, &imageInfo }
			};

			deps.device.updateDescriptorSets ( writes, {} );
		}

		{
			CameraUniformBlock cameraData { glm::identity <glm::mat4> (), glm::identity <glm::mat4> () };

			CreateBuffer ( deps.physicalDevice, deps.device, transferCommandPool, deps.queues->transferQueue,
				BufferUsages::uniformBuffer, &cameraData, sizeof ( CameraUniformBlock ), cameraUniformBuffer, cameraUniformBufferMemory );

			cameraDescriptorSet = AllocateDescriptorSet ( deps.device, descriptorPool, cameraDSetLayout );

			vk::DescriptorBufferInfo bufferInfo { cameraUniformBuffer, 0, sizeof ( CameraUniformBlock ) };
			vk::WriteDescriptorSet write { cameraDescriptorSet, 0, 0, 1, vk::DescriptorType::eUniformBuffer, {}, &bufferInfo };
			deps.device.updateDescriptorSets ( { write }, {} );
		}
	}

	void Axel::Shutdown ()
	{
		deps.device.destroy ( texImageView );
		deps.device.destroy ( texImage );
		deps.device.free ( texMemory );

		deps.device.destroy ( cameraUniformBuffer );
		deps.device.free ( cameraUniformBufferMemory );

		deps.device.destroy ( materialUniformBuffer );
		deps.device.free ( materialUniformBufferMemory );

		deps.device.free ( indexBufferMemory );
		deps.device.destroy ( indexBuffer );
		
		deps.device.free ( vertexBufferMemory );
		deps.device.destroy ( vertexBuffer );
		
		deps.device.destroy ( descriptorPool );
		deps.device.destroy ( transferCommandPool );
		deps.device.destroy ( graphicsPipeline );
		deps.device.destroy ( pipelineLayout );
		
		deps.device.destroy ( cameraDSetLayout );
		deps.device.destroy ( materialDSetLayout );

		deps.device.destroy ( sampler );
	}

	void Axel::LoadScene ( std::string const & filePath )
	{
		assert ( std::filesystem::exists ( filePath ) );

		if ( sceneLoaded )
			UnloadScene ();

		sceneLoaded = true;
		
		objl::Loader loader;
		loader.LoadFile ( filePath );

		std::cout << loader.LoadedMeshes.size () << std::endl;

		std::vector <float> vertices;
		std::vector <uint32_t> indices;

		vertices.reserve ( loader.LoadedVertices.size () * ( 3 + 2 ) );
		indices.reserve ( loader.LoadedIndices.size () );

		for ( auto const & mesh : loader.LoadedMeshes )
		{
			objectInfos.push_back ( { static_cast <int> ( indices.size () ), static_cast <int> ( mesh.Indices.size () ) } );

			// Convert to indices into the global vertex buffer
			for ( auto index : mesh.Indices )
				indices.push_back ( vertices.size () / ( 3 + 2 ) + index );

			// Load vertices as is
			for ( auto const & vertex : mesh.Vertices )
			{
				vertices.push_back ( vertex.Position.X );
				vertices.push_back ( vertex.Position.Y );
				vertices.push_back ( vertex.Position.Z );

				vertices.push_back ( vertex.TextureCoordinate.X );
				vertices.push_back ( vertex.TextureCoordinate.Y );
			}
		}

		indexCount = loader.LoadedIndices.size ();

		CreateBuffer ( deps.physicalDevice, deps.device, transferCommandPool, deps.queues->transferQueue,
			BufferUsages::vertexBuffer, vertices.data (), vertices.size () * sizeof ( float ), vertexBuffer, vertexBufferMemory );

		CreateBuffer ( deps.physicalDevice, deps.device, transferCommandPool, deps.queues->transferQueue,
			BufferUsages::indexBuffer, indices.data (), indices.size () * sizeof ( uint32_t ), indexBuffer, indexBufferMemory );

	}

	void Axel::UnloadScene ()
	{
		sceneLoaded = false;

		deps.device.destroy ( vertexBuffer );
		deps.device.free ( vertexBufferMemory );
		deps.device.destroy ( indexBuffer );
		deps.device.free ( indexBufferMemory );
	}
	
	void Axel::SetCamera ( Camera const & camera )
	{
		CameraUniformBlock cameraData { camera.GetViewMatrix (), camera.GetProjectionMatrix () };
		
		UpdateBuffer ( deps.physicalDevice, deps.device, transferCommandPool, deps.queues->transferQueue, 
			cameraUniformBuffer, &cameraData, sizeof ( CameraUniformBlock ) );
		
		vk::DescriptorBufferInfo bufferInfo { cameraUniformBuffer, 0, sizeof ( CameraUniformBlock ) };
		vk::WriteDescriptorSet write { cameraDescriptorSet, 0, 0, 1, vk::DescriptorType::eUniformBuffer, {}, &bufferInfo };
		deps.device.updateDescriptorSets ( { write }, {} );
	}

	void Axel::RecordRender ( vk::CommandBuffer renderCommandBuffer, vk::Extent2D const & viewportExtent )
	{
		if ( ! sceneLoaded ) return;

		renderCommandBuffer.bindPipeline ( vk::PipelineBindPoint::eGraphics, graphicsPipeline );

		std::vector <vk::Viewport> viewports { { 
			0, 
			static_cast < float > ( viewportExtent.height ), 
			static_cast < float > ( viewportExtent.width ),
			-static_cast < float > ( viewportExtent.height ),
			0.0f, 
			1.0f 
		} };

		std::vector <vk::Rect2D> scissors { { { 0, 0 }, viewportExtent } };

		renderCommandBuffer.setViewport ( 0, viewports );
		renderCommandBuffer.setScissor ( 0, scissors );

		renderCommandBuffer.bindDescriptorSets ( vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, { materialDescriptorSet, cameraDescriptorSet }, {} );
		renderCommandBuffer.bindVertexBuffers ( 0, { vertexBuffer }, { 0 } );
		renderCommandBuffer.bindIndexBuffer ( indexBuffer, 0, vk::IndexType::eUint32 );

		for ( auto const & objectInfo : objectInfos )
		{
			renderCommandBuffer.drawIndexed ( objectInfo.indexCount, 1, objectInfo.indexOffset, 0, 0);
		}

	}
}
