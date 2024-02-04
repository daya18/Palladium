#version 460 core

layout ( location = 0 ) in vec2 i_textureCoordinates;

layout ( location = 0 ) out vec4 o_color;

layout ( set = 1, binding = 0 ) uniform MaterialBlock
{
	vec4 ambientColor;
	vec4 diffuseColor;
	vec4 specularColor;
	vec4 nothingColor;
}
material;

layout ( set = 2, binding = 0 ) uniform sampler u_sampler;
layout ( set = 2, binding = 1 ) uniform texture2D u_ambientTexture;
layout ( set = 2, binding = 2 ) uniform texture2D u_diffuseTexture;
layout ( set = 2, binding = 3 ) uniform texture2D u_specularTexture;

void main ()
{
	vec3 ambient = material.ambientColor.xyz * texture ( sampler2D ( u_ambientTexture, u_sampler ), i_textureCoordinates ).rgb * 0.1f;
	vec3 diffuse = material.diffuseColor.xyz * texture ( sampler2D ( u_diffuseTexture, u_sampler ), i_textureCoordinates ).rgb * 1.0f;

	//o_color = vec4 ( 1, 1, 1, 1 );
	o_color = vec4 ( ambient + diffuse, 1.0 );
	//color = texture ( sampler2D ( u_texture, u_sampler ), i_textureCoordinates );
}