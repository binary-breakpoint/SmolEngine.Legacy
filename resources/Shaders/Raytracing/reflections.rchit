#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

hitAttributeEXT vec2 attribs;

struct hitPayload
{
  vec3 hitValue;
  int  depth;
  vec3 attenuation;
  int  done;
  vec3 rayOrigin;
  vec3 rayDir;
};

layout(location = 0) rayPayloadInEXT hitPayload prd;

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
	uint     materialUUID;  
	uint64_t vertexAddress;    
	uint64_t indexAddress;     
};

struct PushConstantRay
{
  vec4  clearColor;
  vec3  lightPosition;
  float lightIntensity;
  int   lightType;
  int   maxDepth;
};

layout(buffer_reference, scalar) buffer Vertices {Vertex v[]; }; // Positions of an object
layout(buffer_reference, scalar) buffer Indices {ivec3 i[]; }; // Triangle indices

layout(set = 0, binding = 666, scalar) buffer IntsBuffer { ObjBuffer i[]; };

layout(push_constant) uniform _PushConstantRay { PushConstantRay pcRay; };


void main()
{
    ObjBuffer objResource = i[gl_InstanceCustomIndexEXT];

    Indices  indices = Indices(objResource.indexAddress);
    Vertices vertices = Vertices(objResource.vertexAddress);

    // Indices of the triangle
    ivec3 ind = indices.i[gl_PrimitiveID];

    // Vertex of the triangle
    Vertex v0 = vertices.v[ind.x];
    Vertex v1 = vertices.v[ind.y];
    Vertex v2 = vertices.v[ind.z];

    const vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

      // Computing the coordinates of the hit position
    const vec3 pos      = v0.Pos * barycentrics.x + v1.Pos * barycentrics.y + v2.Pos * barycentrics.z;
    const vec3 worldPos = vec3(gl_ObjectToWorldEXT * vec4(pos, 1.0));  // Transforming the position to world space

    // Computing the normal at hit position
    const vec3 nrm      = v0.Normals * barycentrics.x + v1.Normals * barycentrics.y + v2.Normals * barycentrics.z;
    const vec3 worldNrm = normalize(vec3(nrm * gl_WorldToObjectEXT));  // Transforming the normal to world space

     // Vector toward the light
    vec3  L;
    float lightIntensity = 1;
    float lightDistance  = 100000.0;

    L = normalize(pcRay.lightPosition);

    vec3 diffuse = vec3(0.8, 0.8, 1);

    vec3  specular    = vec3(0);
    float attenuation = 1;

    vec3 origin = worldPos;
    vec3 rayDir = reflect(gl_WorldRayDirectionEXT, worldNrm);
    prd.attenuation *= 1.0;
    prd.done      = 0;
    prd.rayOrigin = origin;
    prd.rayDir    = rayDir;

    prd.hitValue = vec3(attenuation * lightIntensity * (diffuse + specular));
}