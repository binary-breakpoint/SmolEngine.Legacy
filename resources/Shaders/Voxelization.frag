#version 450
#extension GL_ARB_shader_image_load_store : require

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

layout (location = 0) in Input
{
	vec2 UV;
	vec3 WorldPos;
    vec3 VoxelPos;
	mat3 TBN;
	flat Material Material;
} In;

layout (location = 23) flat in vec4 triangleBoundingBox;

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

layout (location = 0) out vec4 fragColor;

layout(binding = 0, r32ui) uniform uimage3D AlbedoBuffer;
layout(binding = 1, r32ui) uniform uimage3D NormalBuffer;
layout(binding = 2, r32ui) uniform uimage3D EmissionBuffer;

layout(binding = 3, rgba8) uniform writeonly image3D voxelAlbedo;
layout(binding = 4, rgba8) uniform writeonly image3D voxelNormal;
layout(binding = 5, rgba8) uniform writeonly image3D voxelEmission;

layout(binding = 6, r8) uniform image3D staticVoxelFlag;

layout (binding = 24) uniform sampler2D texturesMap[4096];

vec4 convRGBA8ToVec4(uint val)
{
    return vec4(float((val & 0x000000FF)), 
    float((val & 0x0000FF00) >> 8U), 
    float((val & 0x00FF0000) >> 16U), 
    float((val & 0xFF000000) >> 24U));
}

uint convVec4ToRGBA8(vec4 val)
{
    return (uint(val.w) & 0x000000FF) << 24U | 
    (uint(val.z) & 0x000000FF) << 16U | 
    (uint(val.y) & 0x000000FF) << 8U | 
    (uint(val.x) & 0x000000FF);
}

void imageAtomicRGBA8Avg_ALBEDO(ivec3 coords, vec4 value) 
{
    value.rgb *= 255.0;     
	         
    uint newVal = convVec4ToRGBA8(value);
    uint prevStoredVal = 0;
    uint curStoredVal;
    uint numIterations = 0;

    while((curStoredVal = imageAtomicCompSwap(AlbedoBuffer, coords, prevStoredVal, newVal)) 
            != prevStoredVal
            && numIterations < 255)
    {
        prevStoredVal = curStoredVal;
        vec4 rval = convRGBA8ToVec4(curStoredVal);
        rval.rgb = (rval.rgb * rval.a); // Denormalize
        vec4 curValF = rval + value;    // Add
        curValF.rgb /= curValF.a;       // Renormalize
        newVal = convVec4ToRGBA8(curValF);

        ++numIterations;
    }

    imageStore(voxelAlbedo, coords, convRGBA8ToVec4(newVal));
}

void imageAtomicRGBA8Avg_NORMAL(ivec3 coords, vec4 value) 
{
    value.rgb *= 255.0;     
	         
    uint newVal = convVec4ToRGBA8(value);
    uint prevStoredVal = 0;
    uint curStoredVal;
    uint numIterations = 0;

    while((curStoredVal = imageAtomicCompSwap(NormalBuffer, coords, prevStoredVal, newVal)) 
            != prevStoredVal
            && numIterations < 255)
    {
        prevStoredVal = curStoredVal;
        vec4 rval = convRGBA8ToVec4(curStoredVal);
        rval.rgb = (rval.rgb * rval.a); // Denormalize
        vec4 curValF = rval + value;    // Add
        curValF.rgb /= curValF.a;       // Renormalize
        newVal = convVec4ToRGBA8(curValF);

        ++numIterations;
    }

    imageStore(voxelNormal, coords, convRGBA8ToVec4(newVal));
}

void imageAtomicRGBA8Avg_EMESSIVE(ivec3 coords, vec4 value) 
{
    value.rgb *= 255.0;     
	         
    uint newVal = convVec4ToRGBA8(value);
    uint prevStoredVal = 0;
    uint curStoredVal;
    uint numIterations = 0;

    while((curStoredVal = imageAtomicCompSwap(EmissionBuffer, coords, prevStoredVal, newVal)) 
            != prevStoredVal
            && numIterations < 255)
    {
        prevStoredVal = curStoredVal;
        vec4 rval = convRGBA8ToVec4(curStoredVal);
        rval.rgb = (rval.rgb * rval.a); // Denormalize
        vec4 curValF = rval + value;    // Add
        curValF.rgb /= curValF.a;       // Renormalize
        newVal = convVec4ToRGBA8(curValF);

        ++numIterations;
    }

    imageStore(voxelEmission, coords, convRGBA8ToVec4(newVal));
}

vec3 EncodeNormal(vec3 normal)
{
    return normal * 0.5f + vec3(0.5f);
}

vec3 DecodeNormal(vec3 normal)
{
    return normal * 2.0f - vec3(1.0f);
}

vec4 fetchAlbedoMap() 
{
	if( In.Material.UseAlbedroTex == 1)
	{
		 return vec4(texture(texturesMap[In.Material.AlbedroTexIndex], In.UV).rgb, 1);
	}

	return In.Material.Albedo;
}

vec3 fetchNormalMap() 
{
	if(In.Material.UseNormalTex == 1)
	{  
        vec3 normal = texture(texturesMap[In.Material.NormalTexIndex], In.UV.xy).rgb;
        return normalize(In.TBN * (normal * 2.0 - 1.0));
	}
	else
	{
		return normalize(In.TBN[2]);
	}
}

float fetchMetallicMap() 
{
	if(In.Material.UseMetallicTex == 1)
	{
		return texture(texturesMap[In.Material.MetallicTexIndex], In.UV.xy).r;
	}

	return In.Material.Metalness;
}

float fetchRoughnessMap() 
{
	if(In.Material.UseRoughnessTex == 1)
	{
       return texture(texturesMap[In.Material.RoughnessTexIndex], In.UV.xy).r;
	}

	return In.Material.Roughness;
}

vec4 fetchEmissiveMap() 
{
	if(In.Material.UseEmissiveTex == 1)
	{
		return texture(texturesMap[In.Material.EmissiveTexIndex], In.UV.xy);
	}

	return vec4(In.Material.EmissionStrength);
}

float fetchAOMap() 
{
	if(In.Material.UseAOTex == 1)
	{
		return texture(texturesMap[In.Material.AOTexIndex], In.UV.xy).r;
	}

	return 1.0;
}

void main()
{
if (any(lessThan(gl_FragCoord.xy, triangleBoundingBox.xy))
      || any(greaterThan(gl_FragCoord.xy, triangleBoundingBox.zw))) {

    discard;  // Clip residue corners from conservative rasterization.
  }

	vec3 N = fetchNormalMap(); 		
	vec4 albedo = fetchAlbedoMap();
	vec4 emissive = fetchEmissiveMap();
	float metallic = fetchMetallicMap();
	float roughness = fetchRoughnessMap();
	roughness = max(roughness, 0.04);
    float ao = fetchAOMap();	
	
	// writing coords position
    ivec3 voxelPos = ivec3(uniformVoxelRes * In.VoxelPos);

   // if(flagStaticVoxels == 0)
   // {
   //     bool isStatic = imageLoad(staticVoxelFlag, position).r > 0.0;
    //}


	// average normal per fragments sorrounding the voxel volume
	// bring normal to 0-1 range
	imageAtomicRGBA8Avg_NORMAL(voxelPos,vec4(EncodeNormal(N), 1.0f));

	// average albedo per fragments sorrounding the voxel volume
    imageAtomicRGBA8Avg_ALBEDO(voxelPos, albedo);

	// average emission per fragments sorrounding the voxel volume
	imageAtomicRGBA8Avg_EMESSIVE(voxelPos, emissive);

    // doing a static flagging pass for static geometry voxelization
    //if(flagStaticVoxels == 1)
  //  {
    //    imageStore(staticVoxelFlag, position, vec4(1.0));
    //}

   // imageStore(staticVoxelFlag, ivec3(0, 0, 0), vec4(1.0)); 

	fragColor = albedo;
}