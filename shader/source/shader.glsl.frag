#version 460 core

layout ( location = 0 ) in vec2 i_textureCoordinates;

layout ( location = 0 ) out vec4 color;

layout ( set = 0, binding = 0 ) uniform MaterialBlock
{
	vec4 color;
}
material;

layout ( set = 0, binding = 1 ) uniform sampler u_sampler;

layout ( set = 0, binding = 2 ) uniform texture2D u_texture;

void main ()
{
//	color = vec4 ( 0, 0, 1, 1 );
//	color = material.color;
	color = texture ( sampler2D ( u_texture, u_sampler ), i_textureCoordinates );
}