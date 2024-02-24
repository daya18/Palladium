#version 460 core

layout ( location = 0 ) in vec2 i_position;
layout ( location = 1 ) in vec2 i_textureCoordinates;

layout ( location = 0 ) out vec2 o_textureCoordinates;

layout ( set = 0, binding = 0 ) uniform CameraBlock
{
	mat4 projection;
}
camera;

layout ( push_constant ) uniform PushConstantBlock
{
	layout ( offset = 0 ) mat4 transform;
}
pushConstants;

void main ()
{
	gl_Position = camera.projection * pushConstants.transform * vec4 ( i_position, 0.0f, 1.0f );
	o_textureCoordinates = i_textureCoordinates;
}