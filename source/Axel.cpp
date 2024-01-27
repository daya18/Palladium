#include "Axel.hpp"

#include "OBJ_Loader.h"

namespace pd
{
	void Axel::Initialize ( Dependencies const & deps )
	{
		this->deps = deps;

		materialDSetLayout = CreateDescriptorSetLayout ( deps.device, {}, { { 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment } } );
		cameraDSetLayout = CreateDescriptorSetLayout ( deps.device, {}, { { 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex } } );

		pipelineLayout = CreatePipelineLayout ( deps.device, { materialDSetLayout, cameraDSetLayout } );
		graphicsPipeline = CreateGraphicsPipeline ( { deps.device, deps.renderPass, 0, pipelineLayout } );
		transferCommandPool = deps.device.createCommandPool ( { {}, deps.queues->transferQueueFamilyIndex } );
		descriptorPool = CreateDescriptorPool ( deps.device );

		glm::vec2 halfSize { 100, 100 };

		std::vector <float> vertices {
			-halfSize.x,  halfSize.y, -500.0f,
			-halfSize.x, -halfSize.y, -500.0f,
			 halfSize.x, -halfSize.y, -500.0f,
			 halfSize.x,  halfSize.y, -500.0f
		};

		std::vector <uint32_t> indices {
			0, 1, 2, 2, 3, 0
		};

		indexCount = indices.size ();

		MaterialUniformBlock materialData { { 0.0f, 1.0f, 0.0f, 1.0f } };

		CreateBuffer ( deps.physicalDevice, deps.device, transferCommandPool, deps.queues->transferQueue,
			BufferUsages::vertexBuffer, vertices.data (), vertices.size () * sizeof ( float ), vertexBuffer, vertexBufferMemory );

		CreateBuffer ( deps.physicalDevice, deps.device, transferCommandPool, deps.queues->transferQueue,
			BufferUsages::indexBuffer, indices.data (), indices.size () * sizeof ( uint32_t ), indexBuffer, indexBufferMemory );

		{
			CreateBuffer ( deps.physicalDevice, deps.device, transferCommandPool, deps.queues->transferQueue,
				BufferUsages::uniformBuffer, &materialData, sizeof ( MaterialUniformBlock ), materialUniformBuffer, materialUniformBufferMemory );

			materialDescriptorSet = AllocateDescriptorSet ( deps.device, descriptorPool, materialDSetLayout );

			vk::DescriptorBufferInfo bufferInfo { materialUniformBuffer, 0, sizeof ( MaterialUniformBlock ) };
			vk::WriteDescriptorSet write { materialDescriptorSet, 0, 0, 1, vk::DescriptorType::eUniformBuffer, {}, &bufferInfo };
			deps.device.updateDescriptorSets ( { write }, {} );
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
	}

	void Axel::LoadScene ( std::string const & filePath )
	{
		assert ( std::filesystem::exists ( filePath ) );
		
		objl::Loader loader;
		loader.LoadFile ( filePath );

	}

	void Axel::UnloadScene ()
	{

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

		renderCommandBuffer.bindPipeline ( vk::PipelineBindPoint::eGraphics, graphicsPipeline );

		std::vector <vk::Viewport> viewports { { 0, 0, static_cast < float > ( viewportExtent.width ),
			static_cast < float > ( viewportExtent.height ), 0.0f, 1.0f } };

		std::vector <vk::Rect2D> scissors { { { 0, 0 }, viewportExtent } };

		renderCommandBuffer.setViewport ( 0, viewports );
		renderCommandBuffer.setScissor ( 0, scissors );

		renderCommandBuffer.bindDescriptorSets ( vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, { materialDescriptorSet, cameraDescriptorSet }, {} );
		renderCommandBuffer.bindVertexBuffers ( 0, { vertexBuffer }, { 0 } );
		renderCommandBuffer.bindIndexBuffer ( indexBuffer, 0, vk::IndexType::eUint32 );
		renderCommandBuffer.drawIndexed ( indexCount, 1, 0, 0, 0 );

	}
}
