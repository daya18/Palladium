#pragma once

namespace pd
{
	class Camera
	{
	public:
		void SetViewportSize ( glm::vec2 const & );

		void SetPosition ( glm::vec3 const & );
		void LookAt ( glm::vec3 point );

		void Move ( glm::vec3 const & delta );
		void PitchYaw ( float pitch, float yaw );

		glm::mat4 const & GetViewMatrix () const;
		glm::mat4 const & GetProjectionMatrix () const;

	private:
		vk::Device device;
		glm::vec3 position {};
		glm::vec3 forward { 0.0f, 0.0f, -1.0f };
		glm::mat4 viewMatrix { glm::identity <glm::mat4> () };
		glm::mat4 projectionMatrix { glm::identity <glm::mat4> () };
	};



	// Implementation
	inline glm::mat4 const & Camera::GetViewMatrix () const { return viewMatrix; }
	inline glm::mat4 const & Camera::GetProjectionMatrix () const { return projectionMatrix; }
}