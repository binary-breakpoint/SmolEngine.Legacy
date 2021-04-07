#version 450 core

layout(location = 0) out vec4 color;
layout(location = 1) out vec4 pos;
layout(location = 2) out vec4 normals;

struct VS_OUT_
{
	vec2 uv;
	vec3 pos;
	vec3 normals;
	vec4 color;
	uint texIndex;
};

layout (location = 0) flat in VS_OUT_ vs_in;

layout (binding = 0) uniform sampler2D texturesMap[4096];

void main()
{
	vec4 texColor = texture(texturesMap[vs_in.texIndex], vs_in.uv);

	color = texColor * vs_in.color;
	pos = vec4(vs_in.pos, 1);
	normals = vec4(vs_in.normals, 1);
}