#version 460 core

layout ( location = 0 ) in flat int i_instanceIndex;
layout ( location = 1 ) in vec2 i_textureCoordinates;

layout ( location = 0 ) out vec4 o_color;

struct InstanceData
{
	vec4 color;
	vec4 borderColor;
	vec4 borderSize;
};

layout ( set = 0, binding = 2 ) uniform InstanceDatasBlock
{
	InstanceData instanceDatas [ 10000 ];
}
instanceDatas;

layout ( set = 1, binding = 0 ) uniform sampler samp;
layout ( set = 1, binding = 1 ) uniform texture2D tex;

void main ()
{
	InstanceData instanceData = instanceDatas.instanceDatas [ i_instanceIndex ];

	o_color = instanceData.color;
	o_color *= texture ( sampler2D ( tex, samp ), i_textureCoordinates );

	// Render border
	if ( 
		( i_textureCoordinates.x <= instanceData.borderSize.x || i_textureCoordinates.x >= ( 1 - instanceData.borderSize.y ) )
		|| ( i_textureCoordinates.y <= instanceData.borderSize.z || i_textureCoordinates.y >= ( 1 - instanceData.borderSize.w ) ) )
			o_color = instanceData.borderColor;
}