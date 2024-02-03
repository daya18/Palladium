#version 460 core

layout ( location = 0 ) in vec3 i_position;
layout ( location = 1 ) in vec2 i_textureCoordinates;

layout ( location = 0 ) out vec2 o_textureCoordinates;

layout ( set = 1, binding = 0 ) uniform CameraBlock
{
	mat4 viewMatrix;
	mat4 projectionMatrix;
}
camera;

void main ()
{
	gl_Position = camera.projectionMatrix * camera.viewMatrix * vec4 ( i_position, 1.0f );
	o_textureCoordinates = i_textureCoordinates;
}