#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_nonuniform_qualifier : enable

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

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

struct RayPayload {
	vec3 color;
	float distance;
	vec3 normal;
	float reflector;
};

layout(location = 0) rayPayloadInEXT RayPayload rayPayload;
hitAttributeEXT vec2 attribs;

// clang-format off

layout(buffer_reference, scalar) readonly buffer Vertices {Vertex v_[]; }; // Positions of an object
layout(buffer_reference, scalar) readonly buffer Indices {uvec3 i_[]; }; // Triangle indices

// clang-format on

layout(binding = 666) uniform IntsBuffer { ObjBuffer objects[100]; };
layout(binding = 667) uniform ColorsBuffer { vec4 colors[100]; };

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

	// Interpolate normal
	const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
	vec3 N = v0.Normals * barycentricCoords.x + v1.Normals * barycentricCoords.y + v2.Normals * barycentricCoords.z;
	vec3 normal = normalize(vec3(N.xyz * gl_WorldToObjectEXT));

	vec4 lightPos = vec4(105.0f, 53.0f, 102.0f, 1);
	vec4 color = colors[gl_InstanceCustomIndexEXT];

	// Basic lighting
	vec3 lightVector = normalize(lightPos.xyz);
	float dot_product = max(dot(lightVector, normal), 0.6);
	rayPayload.color = color.rgb * vec3(dot_product);
	rayPayload.distance = gl_RayTmaxEXT;
	rayPayload.normal = normal;

	// Objects with full white vertex color are treated as reflectors
	rayPayload.reflector = ((color.r == 1.0f) && (color.g == 1.0f) && (color.b == 1.0f)) ? 1.0f : 0.0f; ; 
}