#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_nonuniform_qualifier : enable

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

struct hitPayload
{
	vec3 radiance;
	vec3 attenuation;
	int  done;
	vec3 rayOrigin;
	vec3 rayDir;
};

struct Vertex
{
	vec3  Pos;
	vec3  Normals;
	vec3  Tangent;
	vec2  UVs;
	ivec4 jointIndices;
	vec4  jointWeight;
};

struct ObjBuffer
{
	uint64_t vertexAddress;
	uint64_t indexAddress;
	uint64_t materialUIID;
	uint64_t materialIndices;
};

layout(location = 0) rayPayloadInEXT hitPayload prd;
hitAttributeEXT vec3 attribs;

// clang-format off

layout(buffer_reference, scalar) buffer Vertices {Vertex v_[]; }; // Positions of an object
layout(buffer_reference, scalar) buffer Indices {ivec3 i_[]; }; // Triangle indices

layout(binding = 666) buffer IntsBuffer { ObjBuffer objects[]; };

// clang-format on

const float specular = 0.5;
const float shiness = 1.0;

vec3 computeSpecular(vec3 V, vec3 L, vec3 N)
{
	const float kPi        = 3.14159265;
	const float kShininess = max(shiness, 4.0);

	// Specular
	const float kEnergyConservation = (2.0 + kShininess) / (2.0 * kPi);
	V                               = normalize(-V);
	vec3  R                         = reflect(-L, N);
	float specular                  = kEnergyConservation * pow(max(dot(V, R), 0.0), kShininess);

	return vec3(specular * specular);
}

void main()
{
  ObjBuffer objResource = objects[gl_InstanceCustomIndexEXT];

  Indices  indices = Indices(objResource.indexAddress);
  Vertices vertices = Vertices(objResource.vertexAddress);

	// Indices of the triangle
	uvec3 ind = indices.i_[gl_PrimitiveID];

	// Vertex of the triangle
	Vertex v0 = vertices.v_[ind.x];
	Vertex v1 = vertices.v_[ind.y];
	Vertex v2 = vertices.v_[ind.z];

	// Barycentric coordinates of the triangle
	const vec3 barycentrics = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);

	// Computing the normal at hit position
	vec3 N = v0.Normals * barycentrics.x + v1.Normals * barycentrics.y + v2.Normals * barycentrics.z;
	N      = normalize(vec3(N.xyz * gl_WorldToObjectEXT));        // Transforming the normal to world space

	// Computing the coordinates of the hit position
	vec3 P = v0.Pos * barycentrics.x + v1.Pos * barycentrics.y + v2.Pos * barycentrics.z;
	P      = vec3(gl_ObjectToWorldEXT * vec4(P, 1.0));        // Transforming the position to world space

	// Hardocded (to) light direction
	vec3 L = normalize(vec3(1, 1, 1));

	float NdotL = dot(N, L);

	// Fake Lambertian to avoid black
	vec3 diffuse  = vec3(0.7f, 0.7f, 0.7f) * max(NdotL, 0.3);
	vec3 specular = computeSpecular(gl_WorldRayDirectionEXT, L, N);;

  prd.radiance = (diffuse + specular) * (1 - shiness) * prd.attenuation;

  // Reflect
	vec3 rayDir = reflect(gl_WorldRayDirectionEXT, N);
	prd.attenuation *= vec3(0.5);
	prd.rayOrigin = P;
	prd.rayDir    = rayDir;
}