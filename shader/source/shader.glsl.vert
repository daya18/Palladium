#version 460 core

vec2 positions [] = {
	vec2 ( -1, 1 ),
	vec2 ( 0, -1 ),
	vec2 ( 1, 1 ),
};

void main ()
{
	gl_Position = vec4 ( positions [gl_VertexIndex], 0.0f, 1.0f );
}