#version 450 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

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

layout (location = 0) in Vertex
{
    vec3 v_FragPos;
    vec2 v_UV;
    mat3 v_TBN;
    flat Material v_Material;

} In[3];

layout (location = 0) out GeometryOut
{
	vec2 UV;
	vec3 WorldPos;
    vec3 VoxelPos;
	mat3 TBN;
	flat Material Material;
} Out;

layout (std140, binding = 202) uniform Data
{
    mat4 uniformWorldToVoxelsMatrix;
    mat4 uniformViewMatrixX;
    mat4 uniformViewMatrixY;
    mat4 uniformViewMatrixZ;
    mat4 uniformProjectionMatrix;

	int uniformVoxelRes;
	ivec3 pad;
};

layout (location = 23) flat out vec4 triangleBoundingBox;

void main() 
{
  const bool conservatively = false;

  mat4 viewMatrix;
  vec3 absNormal = abs(In[0].v_TBN[2]);
  if (absNormal.x >= absNormal.y && absNormal.x >= absNormal.z) {
    viewMatrix = uniformViewMatrixX;
  } else if (absNormal.y >= absNormal.x && absNormal.y >= absNormal.z) {
    viewMatrix = uniformViewMatrixY;
  } else {
    viewMatrix = uniformViewMatrixZ;
  }

  mat4 viewProjectionMatrix = uniformProjectionMatrix * viewMatrix;
  mat4 viewProjectionInverseMatrix = inverse(viewProjectionMatrix);

  // Conservative rasterization
  vec4 vsProjs[3] = vec4[3] (viewProjectionMatrix * gl_in[0].gl_Position,
                             viewProjectionMatrix * gl_in[1].gl_Position,
                             viewProjectionMatrix * gl_in[2].gl_Position);

  vec4 projNormal = transpose(viewProjectionInverseMatrix) * vec4(In[0].v_TBN[2], 0);

  // Pass axis-aligned, extended bounding box in NDCs.
  triangleBoundingBox = vec4(min(vsProjs[0].xy, min(vsProjs[1].xy, vsProjs[2].xy)),
                             max(vsProjs[0].xy, max(vsProjs[1].xy, vsProjs[2].xy)));
  triangleBoundingBox = (triangleBoundingBox * vec4(0.5) + vec4(0.5)) * uniformVoxelRes;
  triangleBoundingBox.xy -= vec2(1.0);
  triangleBoundingBox.zw += vec2(1.0);

  // Calculate normal equation of triangle plane.
  vec3 normal = cross(vsProjs[1].xyz - vsProjs[0].xyz, vsProjs[0].xyz - vsProjs[2].xyz);
  normal = projNormal.xyz;
  float d = dot(vsProjs[0].xyz, normal);
  float normalSign = (projNormal.z > 0) ? 1.0 : -1.0;

  // Convert edges to homogeneous line equations and dilate triangle.
  // See:  http://http.developer.nvidia.com/GPUGems2/gpugems2_chapter42.html
  vec3 planes[3]; vec3 intersection[3]; float z[3];
  vec2 halfPixelSize = vec2(1.0 / uniformVoxelRes);
  planes[0] = cross(vsProjs[0].xyw - vsProjs[2].xyw, vsProjs[2].xyw);
  planes[1] = cross(vsProjs[1].xyw - vsProjs[0].xyw, vsProjs[0].xyw);
  planes[2] = cross(vsProjs[2].xyw - vsProjs[1].xyw, vsProjs[1].xyw);
  planes[0].z += normalSign * dot(halfPixelSize, abs(planes[0].xy));
  planes[1].z += normalSign * dot(halfPixelSize, abs(planes[1].xy));
  planes[2].z += normalSign * dot(halfPixelSize, abs(planes[2].xy));
  intersection[0] = cross(planes[0], planes[1]);
  intersection[1] = cross(planes[1], planes[2]);
  intersection[2] = cross(planes[2], planes[0]);
  intersection[0] /= intersection[0].z;
  intersection[1] /= intersection[1].z;
  intersection[2] /= intersection[2].z;
  z[0] = (-intersection[0].x * normal.x - intersection[0].y * normal.y + d) / normal.z;
  z[1] = (-intersection[1].x * normal.x - intersection[1].y * normal.y + d) / normal.z;
  z[2] = (-intersection[2].x * normal.x - intersection[2].y * normal.y + d) / normal.z;
  vsProjs[0].xyz = vec3(intersection[0].xy, z[0]);
  vsProjs[1].xyz = vec3(intersection[1].xy, z[1]);
  vsProjs[2].xyz = vec3(intersection[2].xy, z[2]);

  // Pass vertex data.
  for (int i = 0; i < gl_in.length(); i++) {
    if(conservatively) {
      gl_Position = vsProjs[i];
      Out.VoxelPos = (uniformWorldToVoxelsMatrix * viewProjectionInverseMatrix * vsProjs[i]).xyz;
    } else {
      gl_Position = viewProjectionMatrix * gl_in[i].gl_Position;
      Out.VoxelPos = (uniformWorldToVoxelsMatrix * gl_in[i].gl_Position).xyz;
    }

    Out.WorldPos = gl_in[i].gl_Position.xyz / gl_in[i].gl_Position.w;
    Out.TBN = In[i].v_TBN;
    Out.UV = In[i].v_UV;
	Out.Material = In[i].v_Material;
    EmitVertex();
  }
  EndPrimitive();
}