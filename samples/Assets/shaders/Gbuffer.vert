#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec3 a_Tangent;
layout(location = 3) in vec2 a_UV;
layout(location = 4) in ivec4 a_BoneIDs;
layout(location = 5) in vec4 a_Weight;

struct InstanceData
{
	vec4 pos;
	vec4 scale; // w = material ID
};

struct MaterialData
{
	vec4 PBR;

	uint UseAlbedroTex;
	uint UseNormalTex;
	uint UseMetallicTex;
	uint UseRoughnessTex;

	uint UseAOTex;
	uint AlbedroTexIndex;
	uint NormalTexIndex;
	uint MetallicTexIndex;

	uint RoughnessTexIndex;
	uint AOTexIndex;
};

layout(std140, binding = 1) uniform ModelData
{   
	InstanceData instances[1000];
};

layout(std140, binding = 10) uniform CameraData
{   
	mat4 projection;
	mat4 view;
	vec4 camPos;
};

layout(std140, binding = 3) uniform MaterialsBuffer
{   
	MaterialData materials[5];
};

layout(location = 0) out vec3 v_pos;
layout(location = 1) out vec3 v_normals;
layout(location = 2) out uint v_albedroMapIndex;
layout(location = 3) out uint v_normalMapIndex;
layout(location = 4) out uint v_metallicMapIndex;
layout(location = 5) out uint v_roughnessMapIndex;
layout(location = 6) out vec2 v_UV;
layout(location = 7) out mat3 v_TBN;

mat4 scale(vec3 value){
    return mat4(
        vec4(value.x, 0.0, 0.0, 0.0),
        vec4(0.0, value.y, 0.0, 0.0),
        vec4(0.0, 0.0, value.z, 0.0),
        vec4(0.0, 0.0, 0.0, 1.0)
    );
}

mat4 translate(vec3 value){
    return mat4(
        vec4(1.0, 0.0, 0.0, 0.0),
        vec4(0.0, 1.0, 0.0, 0.0),
        vec4(0.0, 0.0, 1.0, 0.0),
        vec4(value.x, value.y, value.z,1.0)
    );
}

void main()
{
	const uint materialID = uint(instances[gl_InstanceIndex].scale.w);
	const mat4 scaleMat = scale(instances[gl_InstanceIndex].scale.xyz);
	const mat4 transMat = translate(instances[gl_InstanceIndex].pos.xyz);
	const mat4 model = transMat * scaleMat;

	v_pos = vec3(model * vec4(a_Position, 1.0));
	v_normals = mat3(model) * a_Normal;
	v_UV = a_UV;

	// Materials 
	v_albedroMapIndex = materials[materialID].AlbedroTexIndex;
	v_normalMapIndex =  materials[materialID].NormalTexIndex;
	v_roughnessMapIndex = materials[materialID].RoughnessTexIndex;
	v_metallicMapIndex = materials[materialID].MetallicTexIndex;

	// TBN matrix
	vec3 T = normalize(vec3(model * vec4(a_Tangent, 0.0)));
	vec3 N = normalize(vec3(model* vec4(a_Normal, 0.0)));
	vec3 B = normalize(vec3(model * vec4(cross(N, T), 0.0)));
	v_TBN = mat3(T, B, N);

    gl_Position =  projection * view * vec4(v_pos, 1.0);
}