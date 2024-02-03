#pragma once

#include "Core.hpp"
#include "Camera.hpp"

namespace pd
{
	class Axel
	{
	public:
		struct Dependencies
		{
			vk::PhysicalDevice physicalDevice;
			vk::Device device;
			DeviceQueues const * queues;
			vk::RenderPass renderPass;
		};

		void Initialize ( Dependencies const & );
		void Shutdown ();

		void LoadScene ( std::string const & filePath );
		void UnloadScene ();

		void SetCamera ( Camera const & );

		void RecordRender ( vk::CommandBuffer, vk::Extent2D const & viewportExtent );

	private:
		struct CameraUniformBlock
		{
			glm::mat4 viewMatrix;
			glm::mat4 projectionMatrix;
		};

		struct MaterialUniformBlock
		{
			glm::vec4 color;
		};

		struct ObjectInfo
		{
			int indexOffset;
			int indexCount;
		};

		Dependencies deps;

		vk::Sampler sampler;
		vk::DescriptorSetLayout materialDSetLayout;
		vk::DescriptorSetLayout cameraDSetLayout;

		vk::PipelineLayout pipelineLayout;
		vk::Pipeline graphicsPipeline;
		vk::CommandPool transferCommandPool;
		vk::DescriptorPool descriptorPool;

		vk::Buffer vertexBuffer {};
		vk::DeviceMemory vertexBufferMemory {};

		uint32_t indexCount { 0 };
		vk::Buffer indexBuffer {};
		vk::DeviceMemory indexBufferMemory {};
		
		vk::Image texImage;
		vk::DeviceMemory texMemory;
		vk::ImageView texImageView;
		vk::Buffer materialUniformBuffer;
		vk::DeviceMemory materialUniformBufferMemory;
		vk::DescriptorSet materialDescriptorSet;

		vk::Buffer cameraUniformBuffer;
		vk::DeviceMemory cameraUniformBufferMemory;
		vk::DescriptorSet cameraDescriptorSet;

		bool sceneLoaded { false };

		std::vector <ObjectInfo> objectInfos;
	};
}