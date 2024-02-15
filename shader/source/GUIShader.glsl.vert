#version 460 core

layout ( location = 0 ) in vec2 i_position;
layout ( location = 1 ) in vec2 i_textureCoordinates;

layout ( location = 0 ) out flat int o_instanceIndex;
layout ( location = 1 ) out vec2 o_textureCoordinates;

layout ( set = 0, binding = 0 ) uniform CameraBlock
{
	mat4 projectionMatrix;
}
camera;

layout ( set = 0, binding = 1 ) uniform InstanceTransformsBlock
{
	mat4 transforms [ 10000 ];
}
instanceTransforms;

layout ( set = 1, binding = 2 ) uniform InstanceIndicesBlock
{
	vec4 indices [10000];
}
instanceIndices;

void main ()
{
	int instanceIndex = int ( instanceIndices.indices [ gl_InstanceIndex ].x );

	gl_Position = camera.projectionMatrix * instanceTransforms.transforms[instanceIndex] * vec4 ( i_position, 0.0f, 1.0f );
	o_instanceIndex = instanceIndex;
	o_textureCoordinates = i_textureCoordinates;
}