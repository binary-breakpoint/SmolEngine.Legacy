#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;

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

layout (location = 0) out vec2 outUV;

void main() 
{
	outUV = inUV;
	gl_Position = sceneData.projection * sceneData.view * vec4(inPos.xyz, 1.0);
}