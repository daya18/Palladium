#include "Axel.hpp"

#include "OBJ_Loader.h"

namespace pd
{
	void Axel::Initialize ( Dependencies const & deps )
	{
		this->deps = deps;

		sampler = CreateDefaultSampler ( deps.device );

		materialDSetLayout = CreateDescriptorSetLayout ( deps.device, {}, {
			{ 0, vk::DescriptorType::eUniformBufferDynamic, 1, vk::ShaderStageFlagBits::eFragment },
		} );
		
		texturesDSetLayout = CreateDescriptorSetLayout ( deps.device, {}, {
			{ 0, vk::DescriptorType::eSampler, 1, vk::ShaderStageFlagBits::eFragment, &sampler },
			{ 1, vk::DescriptorType::eSampledImage, 1, vk::ShaderStageFlagBits::eFragment },
			{ 2, vk::DescriptorType::eSampledImage, 1, vk::ShaderStageFlagBits::eFragment },
			{ 3, vk::DescriptorType::eSampledImage, 1, vk::ShaderStageFlagBits::eFragment }
		} );

		cameraDSetLayout = CreateDescriptorSetLayout ( deps.device, {}, { { 0, vk::DescriptorType::eUniformBuffer, 1,
			vk::ShaderStageFlagBits::eVertex } } );

		pipelineLayout = CreatePipelineLayout ( deps.device, { cameraDSetLayout, materialDSetLayout, texturesDSetLayout } );
		graphicsPipeline = CreateGraphicsPipeline ( { deps.device, deps.renderPass, 0, pipelineLayout } );
		transferCommandPool = deps.device.createCommandPool ( { {}, deps.queues->transferQueueFamilyIndex } );
		descriptorPool = CreateDescriptorPool ( deps.device );

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
		UnloadScene ();

		deps.device.destroy ( cameraUniformBuffer );
		deps.device.free ( cameraUniformBufferMemory );

		deps.device.destroy ( materialUniformBuffer );
		deps.device.free ( materialUniformBufferMemory );

		deps.device.destroy ( descriptorPool );
		deps.device.destroy ( transferCommandPool );
		deps.device.destroy ( graphicsPipeline );
		deps.device.destroy ( pipelineLayout );
		
		deps.device.destroy ( cameraDSetLayout );
		deps.device.destroy ( materialDSetLayout );
		deps.device.destroy ( texturesDSetLayout );
		
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

		std::vector <float> vertices;
		std::vector <uint32_t> indices;
		std::vector <MaterialUniformBlock> materials;
		std::vector <std::string> texturePaths { "image/White.png" };

		struct ObjectTextureIndices
		{
			int ambient;
			int diffuse;
			int specular;
		};

		std::vector < ObjectTextureIndices> textureIndices;
		
		vertices.reserve ( loader.LoadedVertices.size () * ( 3 + 2 ) );
		indices.reserve ( loader.LoadedIndices.size () );
		materials.reserve ( loader.LoadedMaterials.size () );
		texturePaths.reserve ( loader.LoadedMaterials.size () );
		textureIndices.reserve ( loader.LoadedMeshes.size () );

		for ( auto const & mesh : loader.LoadedMeshes )
		{
			ObjectTextureIndices objectTextureIndices { 0, 0, 0 };

			ObjectInfo objectInfo {};

			objectInfo.indexOffset = static_cast < int > ( indices.size () );
			objectInfo.indexCount = static_cast < int > ( mesh.Indices.size () );
			objectInfo.materialUniformBufferOffset = static_cast < uint32_t > ( materials.size () * sizeof ( MaterialUniformBlock ) );

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

			if ( ! mesh.MeshMaterial.name.empty () )
			{
				glm::vec3 ambientColor { mesh.MeshMaterial.Ka.X, mesh.MeshMaterial.Ka.Y, mesh.MeshMaterial.Ka.Z };
				glm::vec3 diffuseColor { mesh.MeshMaterial.Kd.X, mesh.MeshMaterial.Kd.Y, mesh.MeshMaterial.Kd.Z };
				glm::vec3 specularColor { mesh.MeshMaterial.Ks.X, mesh.MeshMaterial.Ks.Y, mesh.MeshMaterial.Ks.Z };

				materials.push_back ( {
					{ ambientColor == glm::zero <glm::vec3> () ? glm::one <glm::vec3> () : ambientColor, 1.0f },
					{ diffuseColor == glm::zero <glm::vec3> () ? glm::one <glm::vec3> () : diffuseColor, 1.0f },
					{ specularColor == glm::zero <glm::vec3> () ? glm::one <glm::vec3> () : specularColor, 1.0f },
				});

				if ( ! mesh.MeshMaterial.map_Ka.empty () )
				{
					objectTextureIndices.ambient = texturePaths.size ();
					texturePaths.push_back ( mesh.MeshMaterial.map_Ka );
				}

				if ( ! mesh.MeshMaterial.map_Kd.empty () )
				{
					objectTextureIndices.diffuse = texturePaths.size ();
					texturePaths.push_back ( mesh.MeshMaterial.map_Kd );
				}
			}
			else
			{
				materials.push_back ( {} );
			}

			textureIndices.push_back ( objectTextureIndices );
			objectInfos.push_back ( objectInfo );
		}

		indexCount = loader.LoadedIndices.size ();

		// Create vertex buffer
		CreateBuffer ( deps.physicalDevice, deps.device, transferCommandPool, deps.queues->transferQueue,
			BufferUsages::vertexBuffer, vertices.data (), vertices.size () * sizeof ( float ), vertexBuffer, vertexBufferMemory );

		// Create index buffer
		CreateBuffer ( deps.physicalDevice, deps.device, transferCommandPool, deps.queues->transferQueue,
			BufferUsages::indexBuffer, indices.data (), indices.size () * sizeof ( uint32_t ), indexBuffer, indexBufferMemory );

		// Create material uniform buffer
		CreateBuffer ( deps.physicalDevice, deps.device, transferCommandPool, deps.queues->transferQueue,
			BufferUsages::uniformBuffer, materials.data (), materials.size () * sizeof ( MaterialUniformBlock ), 
			materialUniformBuffer, materialUniformBufferMemory);

		// Create material descriptor set layout
		materialDescriptorSet = AllocateDescriptorSet ( deps.device, descriptorPool, materialDSetLayout );
		vk::DescriptorBufferInfo bufferInfo { materialUniformBuffer, 0, sizeof ( MaterialUniformBlock ) };
		vk::WriteDescriptorSet write { materialDescriptorSet, 0, 0, 1, vk::DescriptorType::eUniformBufferDynamic, {}, &bufferInfo };
		deps.device.updateDescriptorSets ( { write }, {} );

		for ( auto const & texturePath : texturePaths )
		{
			assert ( std::filesystem::exists ( texturePath ) );

			Texture texture;
			
			CreateTexture ( deps.physicalDevice, deps.device, transferCommandPool,
				deps.queues->transferQueue, deps.queues->transferQueueFamilyIndex,
				texturePath, texture.texImage, texture.texImageView, texture.texMemory );		
			
			textures.push_back ( texture );
		}

		int index { 0 };
		for ( auto const & objectTextureIndices : textureIndices )
		{
			objectInfos [ index ].texturesDescriptorSet = AllocateDescriptorSet ( deps.device, descriptorPool, texturesDSetLayout );

			vk::DescriptorImageInfo ambientImageInfo { {}, textures [ objectTextureIndices.ambient ].texImageView, vk::ImageLayout::eShaderReadOnlyOptimal };
			vk::DescriptorImageInfo diffuseImageInfo { {}, textures [ objectTextureIndices.diffuse ].texImageView, vk::ImageLayout::eShaderReadOnlyOptimal };
			vk::DescriptorImageInfo specularImageInfo { {}, textures [ objectTextureIndices.specular ].texImageView, vk::ImageLayout::eShaderReadOnlyOptimal };

			std::vector <vk::WriteDescriptorSet> writes {
				{ objectInfos [ index ].texturesDescriptorSet, 1, 0, 1, vk::DescriptorType::eSampledImage, & ambientImageInfo },
				{ objectInfos [ index ].texturesDescriptorSet, 2, 0, 1, vk::DescriptorType::eSampledImage, & diffuseImageInfo },
				{ objectInfos [ index ].texturesDescriptorSet, 3, 0, 1, vk::DescriptorType::eSampledImage, & specularImageInfo },
			};

			deps.device.updateDescriptorSets ( writes, {} );

			++index;
		}

	}

	void Axel::UnloadScene ()
	{
		sceneLoaded = false;

		deps.device.destroy ( vertexBuffer );
		deps.device.free ( vertexBufferMemory );
		deps.device.destroy ( indexBuffer );
		deps.device.free ( indexBufferMemory );

		for ( auto const & texture : textures )
		{
			deps.device.destroy ( texture.texImageView );
			deps.device.destroy ( texture.texImage );
			deps.device.free ( texture.texMemory );
		}

		for ( auto const & objectInfo : objectInfos )
		{
			deps.device.free ( descriptorPool, objectInfo.texturesDescriptorSet );
		}
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

		renderCommandBuffer.bindDescriptorSets ( vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, { cameraDescriptorSet }, {} );
		renderCommandBuffer.bindVertexBuffers ( 0, { vertexBuffer }, { 0 } );
		renderCommandBuffer.bindIndexBuffer ( indexBuffer, 0, vk::IndexType::eUint32 );
		 
		for ( auto const & objectInfo : objectInfos )
		{
			renderCommandBuffer.bindDescriptorSets ( vk::PipelineBindPoint::eGraphics, pipelineLayout, 1,
				{ materialDescriptorSet, objectInfo.texturesDescriptorSet }, { objectInfo.materialUniformBufferOffset } );
			
			renderCommandBuffer.drawIndexed ( objectInfo.indexCount, 1, objectInfo.indexOffset, 0, 0);
		}

	}

	//vk::ImageView Axel::GetTexture ( std::string const & filePath )
	//{
	//	//CreateTexture ( deps.physicalDevice, deps.device, transferCommandPool,
	//	//	deps.queues->transferQueue, deps.queues->transferQueueFamilyIndex,
	//	//	"image/TestImage.jpg", objectInfo.texImage, objectInfo.texImageView, objectInfo.texMemory );


	//	//vk::DescriptorImageInfo imageInfo { {}, objectInfo.texImageView, vk::ImageLayout::eShaderReadOnlyOptimal };

	//	//std::vector <vk::WriteDescriptorSet> writes {
	//	//	{ objectInfo.materialDescriptorSet, 0, 0, 1, vk::DescriptorType::eUniformBuffer, {}, &bufferInfo },
	//	//	{ objectInfo.materialDescriptorSet, 2, 0, 1, vk::DescriptorType::eSampledImage, &imageInfo }
	//	//};

	//	//deps.device.updateDescriptorSets ( writes, {} );
	//	//{ materialDescriptorSet, 2, 0, 1, vk::DescriptorType::eSampledImage, &imageInfo }
	//	return {};
	//}
}
