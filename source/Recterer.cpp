#include "Recterer.hpp"

namespace pd
{
	void Recterer::Initialize ( Dependencies const & deps )
	{
		this->deps = deps;

		descriptorPool = CreateDescriptorPool ( deps.device );
		sampler = CreateDefaultSampler ( deps.device );

		globalDescriptorSetLayout = CreateDescriptorSetLayout ( deps.device, {}, { 
			// Camera
			{ 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex },
			// Instance transforms array
			{ 1, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex },
			// Instance color array
			{ 2, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eFragment },
		} );

		batchDescriptorSetLayout = CreateDescriptorSetLayout ( deps.device, {}, {
			// Texture sampler
			{ 0, vk::DescriptorType::eSampler, 1, vk::ShaderStageFlagBits::eFragment, &sampler },
			// Texture
			{ 1, vk::DescriptorType::eSampledImage, 1, vk::ShaderStageFlagBits::eFragment },
			// Instance index array
			{ 2, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex },
		} );

		globalDescriptorSet = AllocateDescriptorSet ( deps.device, descriptorPool, globalDescriptorSetLayout );
		
		pipelineLayout = CreatePipelineLayout ();
		pipeline = CreatePipeline ();

		rectangleIDManager = { 0, maxInstances - 1 };

		CreateGeometryBuffers ();
		
		// Create camera buffer
		CreateBuffer ( deps.physicalDevice, deps.device, BufferUsages::uniformBuffer, sizeof ( CameraData ),
			cameraUniformBuffer, cameraUniformBufferMemory );

		// Create instance transforms buffer
		CreateBuffer ( deps.physicalDevice, deps.device, BufferUsages::uniformBuffer, sizeof ( InstanceTransformsData ),
			instanceTransformsBuffer, instanceTransformsBufferMemory );

		// Create instance colors buffer
		CreateBuffer ( deps.physicalDevice, deps.device, BufferUsages::uniformBuffer, 
			sizeof ( InstanceFragmentShaderData ) * maxInstances, instanceColorsBuffer, instanceColorsBufferMemory );

		{
			vk::DescriptorBufferInfo instanceTransformsBufferInfo { instanceTransformsBuffer, 0, sizeof ( glm::mat4 ) * maxInstances };
			vk::DescriptorBufferInfo instanceColorsBufferInfo { instanceColorsBuffer, 0, sizeof ( glm::vec4 ) * maxInstances };

			std::vector <vk::WriteDescriptorSet> writes {
				{ globalDescriptorSet, 1, 0, 1, vk::DescriptorType::eUniformBuffer, {}, &instanceTransformsBufferInfo },
				{ globalDescriptorSet, 2, 0, 1, vk::DescriptorType::eUniformBuffer, {}, &instanceColorsBufferInfo }
			};

			deps.device.updateDescriptorSets ( writes, {} );
		}

		SetViewportSize ( { 1280, 720 } );
	}

	void Recterer::Shutdown ()
	{
		for ( auto const & [texture, batch] : batches )
			DeleteBatch ( batch );

		deps.device.free ( descriptorPool, globalDescriptorSet );

		deps.device.destroy ( globalDescriptorSetLayout );
		deps.device.destroy ( batchDescriptorSetLayout );
		
		deps.device.destroy ( cameraUniformBuffer );
		deps.device.free ( cameraUniformBufferMemory );
		
		deps.device.destroy ( vertexBuffer );
		deps.device.free ( vertexBufferMemory );

		deps.device.destroy ( indexBuffer );
		deps.device.free ( indexBufferMemory );
		
		deps.device.destroy ( instanceTransformsBuffer );
		deps.device.free ( instanceTransformsBufferMemory );

		deps.device.destroy ( instanceColorsBuffer );
		deps.device.free ( instanceColorsBufferMemory );

		deps.device.destroy ( pipeline );
		deps.device.destroy ( pipelineLayout );

		deps.device.destroy ( sampler );
		deps.device.destroy ( descriptorPool );
	}

	void Recterer::RecordRender ( vk::CommandBuffer commandBuffer, vk::Extent2D viewportExtent )
	{
		commandBuffer.bindPipeline ( vk::PipelineBindPoint::eGraphics, pipeline );

		pd::SetViewport ( commandBuffer, viewportExtent );

		commandBuffer.bindVertexBuffers ( 0, { vertexBuffer }, { 0 } );
		commandBuffer.bindIndexBuffer ( indexBuffer, 0, vk::IndexType::eUint32 );

		commandBuffer.bindDescriptorSets ( vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, { globalDescriptorSet }, {} );
		
		for ( auto const & [ texture, batch ] : batches )
		{
			commandBuffer.bindDescriptorSets ( vk::PipelineBindPoint::eGraphics, pipelineLayout, 1, { batch.descriptorSet }, {} );
			commandBuffer.drawIndexed ( 6, batch.instanceIndices.size (), 0, 0, 0 );
		}
	}
	
	void Recterer::SetViewportSize ( glm::vec2 const & size )
	{
		CameraData cameraData { glm::ortho ( 0.0f, size.x, size.y, 0.0f ) };

		UpdateBuffer ( deps.physicalDevice, deps.device, deps.transferCommandPool,
			deps.queues->transferQueue, cameraUniformBuffer, &cameraData, sizeof ( CameraData ) );

		vk::DescriptorBufferInfo bufferInfo { cameraUniformBuffer, 0, sizeof ( CameraData ) };
		vk::WriteDescriptorSet write { globalDescriptorSet, 0, 0, 1, vk::DescriptorType::eUniformBuffer, {}, &bufferInfo };
		deps.device.updateDescriptorSets ( { write }, {} );
	}

	int Recterer::CreateRectangle ()
	{
		auto id { rectangleIDManager.GetID () };
		
		// Initialize to default state
		SetRectangleTexture ( id, "image/White.png");
		SetRectangleColor ( id, { 1, 1, 1, 1 } );
		SetRectangleTransform ( id, glm::scale ( glm::identity <glm::mat4> (), { 100, 100, 1 } ) );
		SetRectangleBorderSizes ( id, 0.02f, 0.02f, 0.02f, 0.02f );
		SetRectangleBorderColor ( id, { 1.0f, 0.0f, 0.0f, 1.0f } );

		return id;
	}

	void Recterer::DeleteRectangle ( int id )
	{
		rectangleIDManager.FreeID ( id );
		RemoveRectangleFromBatch ( id );
	}

	void Recterer::SetRectangleTransform ( int id, glm::mat4 const & transform )
	{
		UpdateBuffer ( deps.physicalDevice, deps.device, deps.transferCommandPool, deps.queues->transferQueue,
			instanceTransformsBuffer, glm::value_ptr ( transform ), sizeof ( glm::mat4 ), id * sizeof ( glm::mat4 ) );
	}

	void Recterer::SetRectangleColor ( int id, glm::vec4 const & color )
	{
		UpdateBuffer ( deps.physicalDevice, deps.device, deps.transferCommandPool, deps.queues->transferQueue,
			instanceColorsBuffer, glm::value_ptr ( color ), sizeof ( glm::vec4 ), id * sizeof ( InstanceFragmentShaderData ) + 0 );
	}
	
	void Recterer::SetRectangleBorderSizes ( int id, float left, float right, float bottom, float top )
	{
		glm::vec4 sizes { left, right, bottom, top };

		UpdateBuffer ( deps.physicalDevice, deps.device, deps.transferCommandPool, deps.queues->transferQueue,
			instanceColorsBuffer, glm::value_ptr ( sizes ), 
			sizeof ( glm::vec4 ), id * sizeof ( InstanceFragmentShaderData ) + ( sizeof ( glm::vec4 ) * 2 ) );
	}

	void Recterer::SetRectangleBorderColor ( int id, glm::vec4 const & color )
	{
		UpdateBuffer ( deps.physicalDevice, deps.device, deps.transferCommandPool, deps.queues->transferQueue,
			instanceColorsBuffer, glm::value_ptr ( color ),
			sizeof ( glm::vec4 ), id * sizeof ( InstanceFragmentShaderData ) + sizeof ( glm::vec4 ) );
	}

	void Recterer::SetRectangleTexture ( int id, std::string const & texture )
	{
		RemoveRectangleFromBatch ( id );
		AddRectangleToBatch ( id, texture );
	}

	void Recterer::AddRectangleToBatch ( int rectangleId, std::string const & batchTexture )
	{
		auto batchIt { batches.find ( batchTexture ) };

		if ( batchIt == batches.end () )
		{
			Batch batch;

			CreateTexture ( deps.physicalDevice, deps.device, deps.transferCommandPool, deps.queues->transferQueue,
				deps.queues->transferQueueFamilyIndex, batchTexture, batch.texture, batch.textureView, batch.textureMemory );

			batch.descriptorSet = AllocateDescriptorSet ( deps.device, descriptorPool, batchDescriptorSetLayout );

			batchIt = batches.insert ( { batchTexture, batch } ).first;
		
			vk::DescriptorImageInfo imageInfo { {}, batchIt->second.textureView, vk::ImageLayout::eShaderReadOnlyOptimal };

			std::vector <vk::WriteDescriptorSet> writes {
				{ batchIt->second.descriptorSet, 1, 0, 1, vk::DescriptorType::eSampledImage, &imageInfo, {} },
			};

			deps.device.updateDescriptorSets ( writes, {} );
		}
		
		batchIt->second.instanceIndices.push_back ( { rectangleId, 0, 0, 0 } );
		
		deps.device.destroy ( batchIt->second.instanceIndexBuffer );
		deps.device.free ( batchIt->second.instanceIndexBufferMemory );

		CreateBuffer ( deps.physicalDevice, deps.device, deps.transferCommandPool, deps.queues->transferQueue, 
			BufferUsages::uniformBuffer,
			batchIt->second.instanceIndices.data (),
			batchIt->second.instanceIndices.size () * sizeof ( glm::vec4 ),
			batchIt->second.instanceIndexBuffer,
			batchIt->second.instanceIndexBufferMemory
		);

		vk::DescriptorBufferInfo bufferInfo { batchIt->second.instanceIndexBuffer, 0, batchIt->second.instanceIndices.size () * sizeof ( glm::vec4 ) };
		
		std::vector <vk::WriteDescriptorSet> writes {
			{ batchIt->second.descriptorSet, 2, 0, 1, vk::DescriptorType::eUniformBuffer, {}, &bufferInfo },
		};

		deps.device.updateDescriptorSets ( writes, {} );
		
		rectangleTextures [ rectangleId ] = batchTexture;
	}

	void Recterer::RemoveRectangleFromBatch ( int rectangleId )
	{
		if ( rectangleTextures.find ( rectangleId ) == rectangleTextures.end () )
			return;

		auto & batch { batches.find ( rectangleTextures.at ( rectangleId ) )->second };

		if ( batch.instanceIndices.size () == 1 )
		{
			DeleteBatch ( batch );
			batches.erase ( rectangleTextures.at ( rectangleId ) );
			return;
		}

		batch.instanceIndices.erase ( 
			std::find ( batch.instanceIndices.begin (), batch.instanceIndices.end (), 
				glm::vec4 { rectangleId, 0, 0, 0 } ) );

		deps.device.destroy ( batch.instanceIndexBuffer );
		deps.device.free ( batch.instanceIndexBufferMemory );

		CreateBuffer ( deps.physicalDevice, deps.device, deps.transferCommandPool, deps.queues->transferQueue,
			BufferUsages::uniformBuffer,
			batch.instanceIndices.data (),
			batch.instanceIndices.size () * sizeof ( glm::vec4 ),
			batch.instanceIndexBuffer,
			batch.instanceIndexBufferMemory
		);

		vk::DescriptorBufferInfo bufferInfo { batch.instanceIndexBuffer, 0, batch.instanceIndices.size () * sizeof ( glm::vec4 ) };

		std::vector <vk::WriteDescriptorSet> writes {
			{ batch.descriptorSet, 2, 0, 1, vk::DescriptorType::eUniformBuffer, {}, &bufferInfo },
		};

		deps.device.updateDescriptorSets ( writes, {} );

		rectangleTextures.erase ( rectangleId );
	}

	vk::PipelineLayout Recterer::CreatePipelineLayout ()
	{
		std::vector <vk::PushConstantRange> pushConstantRanges {};
		return pd::CreatePipelineLayout ( deps.device, { globalDescriptorSetLayout, batchDescriptorSetLayout }, pushConstantRanges );
	}

	vk::Pipeline Recterer::CreatePipeline ()
	{
		vk::ShaderModule vertexShader { CreateShaderModuleFromFile ( deps.device, "shader/build/GUIShader.spv.vert" ) };
		vk::ShaderModule fragmentShader { CreateShaderModuleFromFile ( deps.device, "shader/build/GUIShader.spv.frag" ) };

		std::vector <vk::PipelineShaderStageCreateInfo> shaderStages
		{
			{ {}, vk::ShaderStageFlagBits::eVertex, vertexShader, "main" },
			{ {}, vk::ShaderStageFlagBits::eFragment, fragmentShader, "main" }
		};

		std::vector <vk::VertexInputBindingDescription> vertexBindings { { 0, sizeof ( float ) * ( 2 + 2 ), vk::VertexInputRate::eVertex } };

		std::vector <vk::VertexInputAttributeDescription> vertexAttributes {
			{ 0, 0, vk::Format::eR32G32Sfloat, 0 },
			{ 1, 0, vk::Format::eR32G32Sfloat, sizeof ( float ) * 2 }
		};

		vk::PipelineVertexInputStateCreateInfo vertexInputState
		{ {}, vertexBindings, vertexAttributes };

		vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState
		{ {}, vk::PrimitiveTopology::eTriangleList, VK_FALSE };

		std::vector <vk::Viewport> viewports { { 0, 0, 1280, 720, 0.0f, 1.0f } };
		std::vector <vk::Rect2D> scissors { { { 0, 0 }, { 1280, 720 } } };

		vk::PipelineViewportStateCreateInfo viewportState { {}, viewports, scissors };

		vk::PipelineRasterizationStateCreateInfo rasterizationState
		{
			{},
			VK_FALSE,
			VK_FALSE,
			vk::PolygonMode::eFill,
			vk::CullModeFlagBits::eNone,
			{},
			{},
			{},
			{},
			{},
			1.0f
		};

		vk::PipelineMultisampleStateCreateInfo multisampleState
		{
			{},
			vk::SampleCountFlagBits::e1,
			VK_FALSE
		};

		std::vector <vk::PipelineColorBlendAttachmentState> colorBlendAttachmentStates
		{
			{
				VK_FALSE,
				{},
				{},
				{},
				{},
				{},
				{},
				vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
			}
		};

		vk::PipelineColorBlendStateCreateInfo colorBlendState
		{
			{},
			VK_FALSE,
			{},
			colorBlendAttachmentStates
		};

		std::vector <vk::DynamicState> dynamicStates { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
		vk::PipelineDynamicStateCreateInfo dynamicState { {}, dynamicStates };

		vk::PipelineDepthStencilStateCreateInfo depthStencilState
		{
			{},
			VK_TRUE,
			VK_TRUE,
			vk::CompareOp::eLess,
			VK_FALSE,
			VK_FALSE
		};

		vk::GraphicsPipelineCreateInfo createInfo
		{
			{},
			shaderStages,
			&vertexInputState,
			&inputAssemblyState,
			nullptr,
			&viewportState,
			&rasterizationState,
			&multisampleState,
			&depthStencilState,
			&colorBlendState,
			&dynamicState,
			pipelineLayout,
			deps.renderPass,
			0,
		};

		auto pipeline { deps.device.createGraphicsPipeline ( {}, createInfo ).value };

		deps.device.destroy ( vertexShader );
		deps.device.destroy ( fragmentShader );

		return pipeline;
	}

	void Recterer::CreateGeometryBuffers ()
	{
		std::vector <float> vertices { 
			0.0f, 0.0f,  0, 1,
			1.0f, 0.0f,  1, 1,
			1.0f, 1.0f,  1, 0,
			0.0f, 1.0f,   0, 0
		};

		std::vector <uint32_t> indices { 0, 1, 2, 2, 3, 0 };

		CreateBuffer ( deps.physicalDevice, deps.device, deps.transferCommandPool, deps.queues->transferQueue, BufferUsages::vertexBuffer,
			vertices.data (), vertices.size () * sizeof ( float ), vertexBuffer, vertexBufferMemory );

		CreateBuffer ( deps.physicalDevice, deps.device, deps.transferCommandPool, deps.queues->transferQueue, BufferUsages::indexBuffer,
			indices.data (), indices.size () * sizeof ( uint32_t ), indexBuffer, indexBufferMemory );
	}

	void Recterer::DeleteBatch ( Batch const & batch )
	{
		deps.device.destroy ( batch.instanceIndexBuffer );
		deps.device.free ( batch.instanceIndexBufferMemory );
		deps.device.destroy ( batch.texture );
		deps.device.destroy ( batch.textureView );
		deps.device.free ( batch.textureMemory );
		deps.device.free ( descriptorPool, batch.descriptorSet );
	}
}