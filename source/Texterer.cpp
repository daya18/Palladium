#include "Texterer.hpp"

namespace pd
{
	void Texterer::Initialize ( Dependencies const & deps )
	{
		this->deps = deps;

		//if ( FT_Init_FreeType ( &ftLibrary ) )
		//	std::cout << "Freetype failed to initialize" << std::endl;

		descriptorPool = CreateDescriptorPool ( deps.device );
		sampler = CreateDefaultSampler ( deps.device );

		globalDescriptorSetLayout = CreateDescriptorSetLayout ( deps.device, {}, {
			// Camera
			{ 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex },
			} );

		instanceDescriptorSetLayout = CreateDescriptorSetLayout ( deps.device, {}, {
			// Texture sampler
			{ 0, vk::DescriptorType::eSampler, 1, vk::ShaderStageFlagBits::eFragment, &sampler },
			// Texture
			{ 1, vk::DescriptorType::eSampledImage, 1, vk::ShaderStageFlagBits::eFragment },
			} );

		globalDescriptorSet = AllocateDescriptorSet ( deps.device, descriptorPool, globalDescriptorSetLayout );

		pipelineLayout = CreatePipelineLayout ();
		pipeline = CreatePipeline ();

		textIDManager = { 0, maxInstances - 1 };

		CreateGeometryBuffers ();

		// Create camera buffer
		CreateBuffer ( deps.physicalDevice, deps.device, BufferUsages::uniformBuffer, sizeof ( CameraData ),
			cameraUniformBuffer, cameraUniformBufferMemory );

		SetViewportSize ( { 1280, 720 } );
	}

	void Texterer::Shutdown ()
	{
		for ( auto & [id, textData] : textDatas )
			DestroyGlyphs ( textData.glyphDatas );

		deps.device.free ( descriptorPool, globalDescriptorSet );

		deps.device.destroy ( globalDescriptorSetLayout );
		deps.device.destroy ( instanceDescriptorSetLayout );

		deps.device.destroy ( cameraUniformBuffer );
		deps.device.free ( cameraUniformBufferMemory );

		deps.device.destroy ( vertexBuffer );
		deps.device.free ( vertexBufferMemory );

		deps.device.destroy ( indexBuffer );
		deps.device.free ( indexBufferMemory );

		deps.device.destroy ( pipeline );
		deps.device.destroy ( pipelineLayout );

		deps.device.destroy ( sampler );
		deps.device.destroy ( descriptorPool );

		//FT_Done_FreeType ( ftLibrary );
	}

	void Texterer::RecordRender ( vk::CommandBuffer commandBuffer, vk::Extent2D viewportExtent )
	{
		commandBuffer.bindPipeline ( vk::PipelineBindPoint::eGraphics, pipeline );

		pd::SetViewport ( commandBuffer, viewportExtent );

		commandBuffer.bindVertexBuffers ( 0, { vertexBuffer }, { 0 } );
		commandBuffer.bindIndexBuffer ( indexBuffer, 0, vk::IndexType::eUint32 );

		// Bind global descriptor set
		commandBuffer.bindDescriptorSets ( vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, { globalDescriptorSet }, {} );

		for ( auto const & [id, textData] : textDatas )
		{
			commandBuffer.pushConstants ( pipelineLayout, vk::ShaderStageFlagBits::eFragment, 64, 16, glm::value_ptr ( textData.color ) );

			for ( auto const & glyphData : textData.glyphDatas )
			{
				commandBuffer.pushConstants ( pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, 64, glm::value_ptr ( glyphData.transform ) );

				commandBuffer.bindDescriptorSets ( vk::PipelineBindPoint::eGraphics, pipelineLayout, 1, { glyphData.descriptorSet }, {} );

				commandBuffer.drawIndexed ( 6, 1, 0, 0, 0 );
			}
		}
	}

	void Texterer::SetViewportSize ( glm::vec2 const & size )
	{
		CameraData cameraData { glm::ortho ( 0.0f, size.x, size.y, 0.0f ) };

		UpdateBuffer ( deps.physicalDevice, deps.device, deps.transferCommandPool,
			deps.queues->transferQueue, cameraUniformBuffer, &cameraData, sizeof ( CameraData ) );

		vk::DescriptorBufferInfo bufferInfo { cameraUniformBuffer, 0, sizeof ( CameraData ) };
		vk::WriteDescriptorSet write { globalDescriptorSet, 0, 0, 1, vk::DescriptorType::eUniformBuffer, {}, &bufferInfo };
		deps.device.updateDescriptorSets ( { write }, {} );
	}

	int Texterer::CreateText ()
	{
		auto id { textIDManager.GetID () };
		textDatas.emplace ( id, TextData {} );
		return id;
	}

	void Texterer::DeleteText ( int id )
	{
		auto & textData { textDatas.at ( id ) };
		textData.text = "";
		LoadGlyphs ( textData );
		textIDManager.FreeID ( id );
	}
	
	void Texterer::SetTextHeight ( int id, float height )
	{
		auto & textData { textDatas.at ( id ) };
		textData.height = height;
		LoadGlyphs ( textData );
	}

	void Texterer::SetText ( int id, std::string const & text )
	{
		auto & textData { textDatas.at ( id ) };
		textData.text = text;
		LoadGlyphs ( textData );
	}

	void Texterer::SetTextFont ( int id, std::string const & font )
	{
		auto & textData { textDatas.at ( id ) };
		textData.font = font;
		LoadGlyphs ( textData );
	}

	void Texterer::SetTextPosition ( int id, glm::vec2 const & position )
	{
		auto & textData { textDatas.at ( id ) };
		textData.position = position;
		//LoadGlyphs ( textData );

		for ( auto & glyphData : textData.glyphDatas )
		{
			glm::mat4 glyphTranslationMat { glm::translate ( glm::identity <glm::mat4> (), { textData.position + glyphData.position, 0.0f } ) };
			glm::mat4 glyphScaleMat { glm::scale ( glm::identity <glm::mat4> (), { glyphData.scale, 1.0f } ) };

			glyphData.transform = glyphTranslationMat * glyphScaleMat;
		}
	}

	void Texterer::SetTextColor ( int id, glm::vec4 const & color )
	{
		auto & textData { textDatas.at ( id ) };
		textData.color = color;
	}

	void Texterer::DestroyGlyphs ( std::vector <GlyphData> & glyphDatas )
	{
		for ( auto const & glyphData : glyphDatas )
		{
			deps.device.destroy ( glyphData.texture );
			deps.device.destroy ( glyphData.textureView );
			deps.device.free ( glyphData.textureMemory );
			deps.device.free ( descriptorPool, glyphData.descriptorSet );
		}

		glyphDatas.clear ();
	}

	glm::vec2 Texterer::GetTextSize ( int id )
	{
		auto & textData { textDatas.at ( id ) };
		return textData.size;
	}

	void Texterer::LoadGlyphs ( TextData & textData )
	{
		DestroyGlyphs ( textData.glyphDatas );

		if ( textData.text.empty () || textData.font.empty () )
			return;

		bt::Face face { btLibrary, textData.font };
		bt::Text text { face, textData.text, static_cast <int> ( textData.height ) };

		textData.size = text.GetSize ();

		for ( auto const & glyph : text.GetGlyphs () )
		{
			if ( glyph.bitmap.buffer )
			{
				GlyphData glyphData {};

				glyphData.position = glyph.position;
				glyphData.scale = glyph.size;

				glm::mat4 glyphTranslationMat { glm::translate ( glm::identity <glm::mat4> (), { textData.position + glyph.position, 0.0f } ) };
				glm::mat4 glyphScaleMat { glm::scale ( glm::identity <glm::mat4> (), { glyph.size, 1.0f } ) };

				glyphData.transform = glyphTranslationMat * glyphScaleMat;

				CreateTexture ( deps.physicalDevice, deps.device, deps.transferCommandPool,
					deps.queues->transferQueue, deps.queues->transferQueueFamilyIndex,
					glyph.bitmap.buffer, { glyph.bitmap.width, static_cast < uint32_t > ( glyph.bitmap.rows ) },
					1,
					glyphData.texture, glyphData.textureView, glyphData.textureMemory );

				glyphData.descriptorSet = AllocateDescriptorSet ( deps.device, descriptorPool, instanceDescriptorSetLayout );

				vk::DescriptorImageInfo imageInfo { {}, glyphData.textureView, vk::ImageLayout::eShaderReadOnlyOptimal };

				std::vector <vk::WriteDescriptorSet> writes {
					{ glyphData.descriptorSet, 1, 0, 1, vk::DescriptorType::eSampledImage, &imageInfo, {} },
				};

				deps.device.updateDescriptorSets ( writes, {} );

				textData.glyphDatas.push_back ( glyphData );
			}
		}
	}

	vk::PipelineLayout Texterer::CreatePipelineLayout ()
	{
		std::vector <vk::PushConstantRange> pushConstantRanges {
			{ vk::ShaderStageFlagBits::eVertex, 0, 64 }, // Transformation matrix
			{ vk::ShaderStageFlagBits::eFragment, 64, 16 }, // Color
		};

		return pd::CreatePipelineLayout ( deps.device, { globalDescriptorSetLayout, instanceDescriptorSetLayout }, pushConstantRanges );
	}

	vk::Pipeline Texterer::CreatePipeline ()
	{
		vk::ShaderModule vertexShader { CreateShaderModuleFromFile ( deps.device, "shader/build/TextShader.spv.vert" ) };
		vk::ShaderModule fragmentShader { CreateShaderModuleFromFile ( deps.device, "shader/build/TextShader.spv.frag" ) };

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
				VK_TRUE,
				vk::BlendFactor::eSrcAlpha,
				vk::BlendFactor::eOneMinusSrcAlpha,
				vk::BlendOp::eAdd,
				vk::BlendFactor::eOne,
				vk::BlendFactor::eOne,
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

	void Texterer::CreateGeometryBuffers ()
	{
		std::vector <float> vertices { 
			0.0f, 0.0f,  0, 0,
			1.0f, 0.0f,  1, 0,
			1.0f, 1.0f,  1, 1,
			0.0f, 1.0f,   0, 1
		};

		std::vector <uint32_t> indices { 0, 1, 2, 2, 3, 0 };

		CreateBuffer ( deps.physicalDevice, deps.device, deps.transferCommandPool, deps.queues->transferQueue, BufferUsages::vertexBuffer,
			vertices.data (), vertices.size () * sizeof ( float ), vertexBuffer, vertexBufferMemory );

		CreateBuffer ( deps.physicalDevice, deps.device, deps.transferCommandPool, deps.queues->transferQueue, BufferUsages::indexBuffer,
			indices.data (), indices.size () * sizeof ( uint32_t ), indexBuffer, indexBufferMemory );
	}
}