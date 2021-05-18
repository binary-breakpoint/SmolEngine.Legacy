#version 450 core

layout(location = 0) in vec3 v_pos;
layout(location = 1) in vec3 v_normals;
layout(location = 2) flat in uint v_albedroMapIndex;
layout(location = 3) flat in uint v_normalMapIndex;
layout(location = 4) flat in uint v_metallicMapIndex;
layout(location = 5) flat in uint v_roughnessMapIndex;
layout(location = 6) in vec2 v_UV;
layout(location = 7) in mat3 v_TBN;

layout(location = 0) out vec4 albedro;
layout(location = 1) out vec4 position;
layout(location = 2) out vec4 normals;

layout (binding = 4) uniform sampler2D texturesMap[4096];

void main()
{
	const float metallness = texture(texturesMap[v_metallicMapIndex], v_UV).r;
	const float roughness = texture(texturesMap[v_roughnessMapIndex], v_UV).r;
	const vec4 color = texture(texturesMap[v_albedroMapIndex], v_UV);

	albedro = color;
	position = vec4(v_pos, metallness);
	normals = vec4(v_normals, roughness);
}