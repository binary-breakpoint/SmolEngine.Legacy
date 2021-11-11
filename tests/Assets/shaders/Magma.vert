#version 460 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec3 a_Tangent;
layout(location = 3) in vec2 a_UV;
layout(location = 4) in ivec4 a_BoneIDs;
layout(location = 5) in vec4 a_Weight;

struct InstanceData
{
	uint matID;
	uint isAnimated;
	uint animOffset;
	uint entityID;
	mat4 model;
};

layout(std140, binding = 25) readonly buffer InstancesBuffer
{   
	InstanceData instances[];
};

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

layout(std430, binding = 28) readonly buffer JointMatrices
{
	mat4 joints[];
};

layout(push_constant) uniform ConstantData
{
	uint dataOffset;
    float time;
};


layout (location = 0)  out vec3 v_FragPos;
layout (location = 1)  out vec3 v_Normal;
layout (location = 2)  out vec2 v_UV;
layout (location = 3)  out float v_LinearDepth;
layout (location = 4)  out float v_Time;

void main()
{
	const uint instanceID = dataOffset + gl_InstanceIndex;
	const mat4 model = instances[instanceID].model;
	const uint animOffset = instances[instanceID].animOffset;
	const bool isAnimated = bool(instances[instanceID].isAnimated);

	mat4 skinMat = mat4(1.0);
	if(isAnimated)
	{
		skinMat = 
		a_Weight.x * joints[animOffset + uint(a_BoneIDs.x)] +
		a_Weight.y * joints[animOffset + uint(a_BoneIDs.y)] +
		a_Weight.z * joints[animOffset + uint(a_BoneIDs.z)] +
		a_Weight.w * joints[animOffset + uint(a_BoneIDs.w)];
	}

	const mat4 modelSkin = model * skinMat;

	v_FragPos = vec3(modelSkin *  vec4(a_Position, 1.0));
	v_UV = a_UV;
	v_LinearDepth = -(sceneData.view * vec4(v_FragPos, 1)).z;
    v_Time = time;
    v_Normal = mat3(transpose(inverse(modelSkin))) * a_Normal;	

	gl_Position =  sceneData.projection * sceneData.view  * vec4(v_FragPos, 1.0);

}