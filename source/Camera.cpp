#include "Camera.hpp"

namespace pd
{
	void Camera::SetViewportSize ( glm::vec2 const & size )
	{
		projectionMatrix = glm::perspective ( glm::radians ( 45.0f ), size.x / size.y, 0.001f, 1000.0f );
	}
}