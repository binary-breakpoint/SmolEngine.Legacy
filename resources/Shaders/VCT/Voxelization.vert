#version 460 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec3 a_Tangent;
layout(location = 3) in vec2 a_UV;
layout(location = 4) in ivec4 a_BoneIDs;
layout(location = 5) in vec4 a_Weight;

struct Material
{
	vec4  Albedo;

	float Metalness;
	float Roughness;
	float EmissionStrength;
	uint  UseAlbedroTex;

	uint  UseNormalTex;
	uint  UseMetallicTex;
	uint  UseRoughnessTex;
    uint  UseAOTex;

	uint UseEmissiveTex;
	uint AlbedroTexIndex;
	uint NormalTexIndex;
	uint MetallicTexIndex;

	uint RoughnessTexIndex;
	uint AOTexIndex;
	uint EmissiveTexIndex;
	uint ID;
};

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

layout(std140, binding = 26) readonly buffer MaterialsBuffer
{   
	Material materials[];
};

layout(std430, binding = 28) readonly buffer JointMatrices
{
	mat4 joints[];
};

layout(push_constant) uniform ConstantData
{
	uint dataOffset;
};

Material GetMaterial(uint id)
{
	for(int i = 0; i < materials.length(); i++)
	{
		if(materials[i].ID == id)
		{
			return materials[i];
		}
	}
}

layout (location = 0) out Vertex
{
    vec3 v_FragPos;
    vec2 v_UV;
    mat3 v_TBN;
    flat Material v_Material;
};

void main()
{
	const uint instanceID = dataOffset + gl_InstanceIndex;
	const mat4 model = instances[instanceID].model;
	const uint materialID = instances[instanceID].matID;
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
	v_Material = GetMaterial(materialID);

	{
		vec3 normal = mat3(transpose(inverse(modelSkin))) * a_Normal;
		vec3 tangent = normalize(vec3(modelSkin * vec4(a_Tangent, 0.0)));
		vec3 B = normalize(vec3(vec4(cross(normal, tangent), 0.0)));

		v_TBN =  mat3(tangent, B, normal);
	}	

	gl_Position = vec4(v_FragPos, 1.0);
}