#version 460 core

layout ( location = 0 ) out vec4 color;

layout ( set = 0, binding = 0 ) uniform MaterialBlock
{
	vec4 color;
}
material;

void main ()
{
//	color = vec4 ( 0, 0, 1, 1 );
	color = material.color;
}