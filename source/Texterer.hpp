#pragma once

#include "Core.hpp"
#include "IDManager.hpp"

#include <freetype/freetype.h>

namespace pd
{
	class Texterer
	{
	public:
		struct Dependencies
		{
			vk::PhysicalDevice physicalDevice;
			vk::Device device;
			DeviceQueues const * queues;
			vk::RenderPass renderPass;
			vk::CommandPool transferCommandPool;
		};

		void Initialize ( Dependencies const & );
		void Shutdown ();

		void RecordRender ( vk::CommandBuffer, vk::Extent2D viewportExtent );
		
		void SetViewportSize ( glm::vec2 const & size );

		int CreateText ();
		void SetText ( int id, std::string const & );
		void SetTextFont ( int id, std::string const & );
		void SetTextPosition ( int id, glm::vec2 const & );
		void SetTextColor ( int id, glm::vec4 const & color );
		void DeleteText ( int id );

	private:
		static inline constexpr int maxInstances { 10000 };

		//struct Batch
		//{
		//	std::string font;

		//};

		struct GlyphData
		{
			// Texture
			vk::Image texture;
			vk::DeviceMemory textureMemory;
			vk::ImageView textureView;

			vk::DescriptorSet descriptorSet;

			glm::mat4 transform;
		};

		struct TextData
		{
			std::string text	{ "" };
			std::string font	{ "font/Roboto/Roboto-Regular.ttf" };
			glm::vec4 color		{ 1.0f, 1.0f, 1.0f, 1.0f };
			glm::vec2 position	{ 0.0f, 0.0f };

			std::vector <GlyphData> glyphDatas;
		};

		struct CameraData
		{
			glm::mat4 projectionMatrix;
		};

		void DestroyGlyphs ( std::vector <GlyphData> & );
		void LoadGlyphs ( TextData & );
		vk::PipelineLayout CreatePipelineLayout ();
		vk::Pipeline CreatePipeline ();
		void CreateGeometryBuffers ();


		Dependencies deps;
		
		FT_Library ftLibrary;

		vk::DescriptorPool descriptorPool;
		vk::Sampler sampler;

		vk::DescriptorSetLayout globalDescriptorSetLayout;
		vk::DescriptorSetLayout instanceDescriptorSetLayout;

		vk::PipelineLayout pipelineLayout;
		vk::Pipeline pipeline;

		vk::Buffer vertexBuffer;
		vk::DeviceMemory vertexBufferMemory;
		vk::Buffer indexBuffer;
		vk::DeviceMemory indexBufferMemory;
		
		vk::Buffer cameraUniformBuffer;
		vk::DeviceMemory cameraUniformBufferMemory;

		vk::DescriptorSet globalDescriptorSet;
		
		IDManager textIDManager;

		std::unordered_map <int, TextData> textDatas;
	};
}