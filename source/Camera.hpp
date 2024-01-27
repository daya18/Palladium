#pragma once

namespace pd
{
	class Camera
	{
	public:
		void SetViewportSize ( glm::vec2 const & );

		glm::mat4 const & GetViewMatrix () const;
		glm::mat4 const & GetProjectionMatrix () const;

	private:
		vk::Device device;
		glm::mat4 viewMatrix { glm::identity <glm::mat4> () };
		glm::mat4 projectionMatrix { glm::identity <glm::mat4> () };
	};



	// Implementation
	inline glm::mat4 const & Camera::GetViewMatrix () const { return viewMatrix; }
	inline glm::mat4 const & Camera::GetProjectionMatrix () const { return projectionMatrix; }
}