#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec4 a_Tangent;
layout(location = 3) in vec2 a_UV;
layout(location = 4) in ivec4 a_BoneIDs;
layout(location = 5) in vec4 a_Weight;

struct Instance
{
	mat4 model;
	vec4 color;
	ivec4 params; // x - texture index
};

struct SceneData
{
	mat4 projection;
	mat4 view;
	mat4 skyBoxMatrix;
	vec4 camPos;
	vec4 params;
};

layout (std140, binding = 27) uniform SceneDataBuffer
{
    SceneData sceneData;
};

layout(std140, binding = 1) readonly buffer ShaderDataBuffer
{   
	Instance data[];
};

struct VS_OUT_
{
	vec2 uv;
	vec3 pos;
	vec3 normals;
	vec4 color;
	uint texIndex;
};

layout (location = 0) out VS_OUT_ vs_out;

void main()
{
	mat4 model = data[gl_InstanceIndex].model;

	vs_out.color = data[gl_InstanceIndex].color;
	vs_out.texIndex = data[gl_InstanceIndex].params.x;
	vs_out.pos = vec3(model * vec4(a_Position, 1.0));
	vs_out.normals = mat3(model) * a_Normal;
	vs_out.uv = a_UV;

    gl_Position =  sceneData.projection * sceneData.view * vec4(vs_out.pos, 1.0);
}