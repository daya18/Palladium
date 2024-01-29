#include "Camera.hpp"

namespace pd
{
	void Camera::SetViewportSize ( glm::vec2 const & size )
	{
		projectionMatrix = glm::perspective ( glm::radians ( 45.0f ), size.x / size.y, 0.001f, 1000.0f );
	}

	void Camera::SetPosition ( glm::vec3 const & position )
	{
		this->position = position;
		viewMatrix = glm::lookAt ( position, position + forward, { 0.0f, 1.0f, 0.0f } );
	}

	void Camera::Move ( glm::vec3 const & delta )
	{
		glm::vec3 newPosition { position };

		newPosition += glm::cross ( forward, glm::vec3 { 0.0f, 1.0f, 0.0f } ) * delta.x;
		newPosition += glm::cross ( forward, glm::vec3 { 1.0f, 0.0f, 0.0f } ) * delta.y;
		newPosition += forward * delta.z;

		SetPosition ( newPosition );
	}

	void Camera::LookAt ( glm::vec3 point )
	{
		this->forward = point;
		viewMatrix = glm::lookAt ( position, position + forward, { 0.0f, 1.0f, 0.0f } );
	}

	void Camera::PitchYaw ( float pitch, float yaw )
	{
		glm::vec3 lookDir { forward };

		lookDir = glm::rotate ( lookDir, glm::radians ( yaw ), { 0.0f, -1.0f, 0.0f } );
		lookDir = glm::rotate ( lookDir, glm::radians ( pitch ), -glm::cross ( forward, glm::vec3 { 0.0f, 1.0f, 0.0f } ) );

		LookAt ( lookDir );
	}

}