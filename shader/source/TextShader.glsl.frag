#version 460 core

layout ( location = 0 ) in vec2 i_textureCoordinates;

layout ( location = 0 ) out vec4 o_color;

layout ( set = 1, binding = 0 ) uniform sampler samp;
layout ( set = 1, binding = 1 ) uniform texture2D tex;

layout ( push_constant ) uniform PushConstantBlock
{
	layout ( offset = 64 ) vec4 color;
}
pushConstants;

void main ()
{
	float mask = texture ( sampler2D ( tex, samp ), i_textureCoordinates ).r;

	o_color = vec4 ( pushConstants.color.xyz, pushConstants.color.a * mask );
}