#pragma once

#include "Core.hpp"
#include "IDManager.hpp"

namespace pd
{
	class Recterer
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

		int CreateRectangle ();
		void DeleteRectangle ( int id );
		void SetRectangleTransform ( int id, glm::mat4 const & );
		void SetRectangleColor ( int id, glm::vec4 const & );
		void SetRectangleTexture ( int id, std::string const & texture );
		void SetRectangleBorderSizes ( int id, float left, float right, float bottom, float top );
		void SetRectangleBorderColor ( int id, glm::vec4 const & );

	private:
		static inline constexpr int maxInstances { 10000 };

		struct Batch
		{
			vk::Image texture;
			vk::DeviceMemory textureMemory;
			vk::ImageView textureView;

			std::vector <glm::vec4> instanceIndices;
			vk::Buffer instanceIndexBuffer {};
			vk::DeviceMemory instanceIndexBufferMemory {};

			vk::DescriptorSet descriptorSet;
		};

		struct CameraData
		{
			glm::mat4 projectionMatrix;
		};

		struct InstanceTransformsData
		{
			glm::mat4 transforms [ maxInstances ];
		};

		struct InstanceFragmentShaderData
		{
			glm::vec4 color;
			glm::vec4 borderColor;
			glm::vec4 borderSizes;
		};

		/*struct InstanceColorsData
		{
			glm::vec4 colors [ maxInstances ];
		};*/

		void AddRectangleToBatch ( int rectangleId, std::string const & batchTexture );
		void RemoveRectangleFromBatch ( int rectangleId );

		Batch CreateBatch ( std::string const & texture, std::vector <int> instanceIndices );
		void DeleteBatch ( Batch const & );

		vk::PipelineLayout CreatePipelineLayout ();
		vk::Pipeline CreatePipeline ();
		void CreateGeometryBuffers ();


		Dependencies deps;
		
		vk::DescriptorPool descriptorPool;
		vk::Sampler sampler;

		vk::DescriptorSetLayout globalDescriptorSetLayout;
		vk::DescriptorSetLayout batchDescriptorSetLayout;

		vk::PipelineLayout pipelineLayout;
		vk::Pipeline pipeline;

		vk::Buffer vertexBuffer;
		vk::DeviceMemory vertexBufferMemory;
		vk::Buffer indexBuffer;
		vk::DeviceMemory indexBufferMemory;
		
		vk::Buffer cameraUniformBuffer;
		vk::DeviceMemory cameraUniformBufferMemory;

		vk::Buffer instanceTransformsBuffer;
		vk::DeviceMemory instanceTransformsBufferMemory;

		vk::Buffer instanceColorsBuffer;
		vk::DeviceMemory instanceColorsBufferMemory;

		vk::DescriptorSet globalDescriptorSet;
		
		IDManager rectangleIDManager;

		std::unordered_map < std::string, Batch> batches;
		std::unordered_map < int, std::string > rectangleTextures;
		friend class Rectangle;
	};
}