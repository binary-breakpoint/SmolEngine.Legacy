#version 450 core

layout(location = 0) in vec3 a_Position;

layout (std140, binding = 27) uniform SceneBuffer
{
	mat4 projection;
	mat4 view;
	vec4 camPos;
	float nearClip;
    float farClip;
	vec2  pad1;
	mat4 skyBoxMatrix;

} sceneData;

layout(location = 0) out vec3 v_WorldPos;

void main()
{
	v_WorldPos = a_Position;
	v_WorldPos.xy *= -1.0;
	
	gl_Position = sceneData.projection * sceneData.skyBoxMatrix * vec4(a_Position.xyz, 1);
}
