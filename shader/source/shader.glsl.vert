#version 460 core

layout ( location = 0 ) in vec3 position;

layout ( set = 1, binding = 0 ) uniform CameraBlock
{
	mat4 viewMatrix;
	mat4 projectionMatrix;
}
camera;

void main ()
{
	gl_Position = camera.projectionMatrix * camera.viewMatrix * vec4 ( position, 1.0f );
}