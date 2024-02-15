#version 460 core

layout ( location = 0 ) in flat int i_instanceIndex;
layout ( location = 1 ) in vec2 i_textureCoordinates;

layout ( location = 0 ) out vec4 o_color;

layout ( set = 0, binding = 2 ) uniform InstanceColorsBlock
{
	vec4 colors [ 10000 ];
}
instanceColors;

layout ( set = 1, binding = 0 ) uniform sampler samp;
layout ( set = 1, binding = 1 ) uniform texture2D tex;

void main ()
{
	o_color = instanceColors.colors [ i_instanceIndex ];
	o_color *= texture ( sampler2D ( tex, samp ), i_textureCoordinates );
}