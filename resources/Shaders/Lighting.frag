#version 460

layout (location = 0) in vec2 inUV;

// Out
layout (location = 0) out vec4 outColor;

// Samplers
layout (binding = 1) uniform sampler2D shadowMap;
layout (binding = 2) uniform samplerCube samplerIrradiance;
layout (binding = 3) uniform sampler2D samplerBRDFLUT;
layout (binding = 4) uniform samplerCube prefilteredMap;
layout (binding = 5) uniform sampler2D albedroMap;
layout (binding = 6) uniform sampler2D positionsMap;
layout (binding = 7) uniform sampler2D normalsMap;
layout (binding = 8) uniform sampler2D materialsMap;

// Buffers
// -----------------------------------------------------------------------------------------------------------------------
struct SpotLight 
{
    vec4 position;
	vec4 direction;
	vec4 color;
	
	float intensity;
	float cutOff;
	float outerCutOff;
    float raduis;
	
	float bias;
	bool  is_active;
};

struct PointLight 
{
    vec4 position;
    vec4 color;
	
	float intensity;
    float raduis;
	float bias;
    bool  is_active;
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

layout(std140, binding = 30) readonly buffer PointLightBuffer
{   
	PointLight pointLights[];
};

layout(std140, binding = 31) readonly buffer SpotLightBuffer
{   
	SpotLight spotLights[];
};

layout(std140, binding = 32) uniform DirLightBuffer
{   
	vec4 direction;
	vec4 color;
	float intensity;
	float bias;
	float zNear;
	float zFar;
	float lightFOV;
	bool  is_active;
	bool  is_cast_shadows;
	bool  is_use_soft_shadows;
} dirLight;


layout(std140, binding = 33) uniform LightingProperties
{   
	vec4  ambientColor;
	float iblScale;
	bool  is_active;

} sceneState;

layout(std140, binding = 34) uniform BloomProperties
{   
	float  Threshold;
	float  Knee;
	float  UpsampleScale;
	float  Intensity;
	float  DirtIntensity;
	float  Exposure;
	float  SkyboxMod;
	bool   Enabled;
} bloomState;

const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, -0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 
);

layout(push_constant) uniform ConstantData
{
	mat4 lightSpace;
	uint numPointsLights;
    uint numSpotLights; 
};

// PBR functions
// -----------------------------------------------------------------------------------------------------------------------
const float PI = 3.14159265359;
const float Epsilon = 0.00001;

float DistributionGGX(vec3 N, vec3 H, float a)
{
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float nom    = a2;
    float denom  = (NdotH2 * (a2 - 1.0) + 1.0);
    denom        = PI * denom * denom;
	
    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}  

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float insideBox3d(vec3 v, vec3 bl, vec3 br)
{
	vec3 s = step(bl, v) - step(br, v);
	return s.x * s.y * s.z;
}

float textureProj(vec4 shadowCoord, vec2 off)
{
	float shadow = 1.0;
	if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 ) 
	{
		float dist = texture( shadowMap, shadowCoord.st + off ).r;
		if ( shadowCoord.w > 0.0 && dist < shadowCoord.z ) 
		{
			shadow = 0.1;
		}
	}
	return shadow;
}

float filterPCF(vec4 sc)
{
	ivec2 texDim = textureSize(shadowMap, 0);
	float scale = 1.5;
	float dx = scale * 1.0 / float(texDim.x);
	float dy = scale * 1.0 / float(texDim.y);

	float shadowFactor = 0.0;
	int count = 0;
	int range = 1;
	
	for (int x = -range; x <= range; x++)
	{
		for (int y = -range; y <= range; y++)
		{
			shadowFactor += textureProj(sc, vec2(dx*x, dy*y));
			count++;
		}
	
	}
	return shadowFactor / count;
}

vec3 FresnelSchlick(float cosTheta, vec3 baseReflectivity) {
	return max(baseReflectivity + (1.0 - baseReflectivity) * pow(2, (-5.55473 * cosTheta - 6.98316) * cosTheta), 0.0);
}

vec3 CalcIBL(vec3 fragToView, vec3 baseReflectivity, vec3 reflectionVec, vec3 normal, vec3 albedo,  vec3 ambient, float metallic, float unclampedRoughness, float roughness, float ao)
{
	vec3 specularRatio = FresnelSchlick(max(dot(normal, fragToView), 0.0), baseReflectivity);
	vec3 diffuseRatio = vec3(1.0) - specularRatio;
	diffuseRatio *= 1.0 - metallic;

    vec3 indirectDiffuse = texture(samplerIrradiance, vec3(normal.x, -normal.y, normal.z)).rgb * albedo * diffuseRatio;
	int specularTextureLevels = textureQueryLevels(prefilteredMap);
	vec3 prefilterColour = textureLod(prefilteredMap, reflectionVec, unclampedRoughness * (specularTextureLevels - 1)).rgb;
	vec2 brdfIntegration = texture(samplerBRDFLUT, vec2(max(dot(normal, fragToView), 0.0), roughness)).rg;
	vec3 indirectSpecular = prefilterColour * (specularRatio * brdfIntegration.x + brdfIntegration.y);

	vec3 result = (indirectDiffuse + indirectSpecular) * ao;
	return result * sceneState.ambientColor.rgb;
}

vec3 CalcDirLight(vec3 V, vec3 N, vec3 F0, vec3 albedo_color, float metallic_color, float roughness_color, vec3 modelPos)
{
	vec3 L = normalize(dirLight.direction.xyz - modelPos);
    vec3 H = normalize(V + L);
	float NdotL = max(dot(N, L), 0.0);       
	
    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness_color);   
    float G   = GeometrySmith(N, V, L, roughness_color);      
    vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), F0);
	
    vec3 kd = mix(vec3(1.0) - F, vec3(0.0), metallic_color);
	vec3 diffuseBRDF = kd * albedo_color;
	vec3 specularBRDF = (F * NDF * G) / max(Epsilon, 4.0 * NdotL);
	vec3 rad =  dirLight.intensity * (5.0 * dirLight.color.rgb);
	return ((albedo_color / PI + diffuseBRDF + specularBRDF) * NdotL) * rad;
}

vec3 CalcPointLight(PointLight light, vec3 V, vec3 N, vec3 F0, vec3 albedo_color, float metallic_color, float roughness_color, vec3 modelPos)
{           
    float distance = length(light.position.xyz - modelPos);
	if(distance > light.raduis) return vec3(0);

	vec3 L = normalize(light.position.xyz - modelPos);
    vec3 H = normalize(V + L);
	float NdotL = max(dot(N, L), 0.0);       
	
	//float attenuation = 1 * ((distance + light.quadratic/light.linear) * (distance * distance)) / (light.linear * distance);

    float attenuation;// = smoothstep(light.raduis, 0, distance);//1.0 - (distance / (light.raduis));
	attenuation = clamp(1.0 - distance/light.raduis, 0.0, 1.0); attenuation *= attenuation;
	//attenuation = clamp(1.0 - distance*distance/(light.raduis*light.raduis), 0.0, 1.0); attenuation *= attenuation;
	//if(attenuation < 0) attenuation = 0;
	//if(attenuation > 1) attenuation = 1;
	
    vec3 radiance = vec3(1.0) * attenuation * light.intensity;
	
    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness_color);   
    float G   = GeometrySmith(N, V, L, roughness_color);      
    vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), F0);
	
    vec3 kd = mix(vec3(1.0) - F, vec3(0.0), metallic_color);
	vec3 diffuseBRDF = kd * albedo_color;
	vec3 specularBRDF = (F * NDF * G) / max(Epsilon, 4.0 * NdotL * 0.8);      
	
	return (albedo_color / PI + diffuseBRDF + specularBRDF) * radiance * NdotL * light.color.rgb;
}

vec3 CalcSpotLight(SpotLight light, vec3 V, vec3 N, vec3 F0, vec3 albedo_color, float metallic_color, float roughness_color, vec3 modelPos)
{
    float distance = length(light.position.xyz - modelPos);
	if(distance > light.raduis) return vec3(0);
	
	vec3 L = normalize(light.position.xyz - modelPos);
	float theta = dot(L, normalize(-light.direction.xyz));
	
	if(theta > light.outerCutOff) return vec3(0); // we're working with angles as cosines instead of degrees so a '>' is used.
	
	// spotlight (soft edges)
    float epsilon = (light.cutOff - light.outerCutOff);
    float intensitys = clamp((light.outerCutOff - theta) / epsilon, 0.0, 1.0);
	vec3 H = normalize(V + L);
	float NdotL = max(dot(N, L), 0.0);   

    float attenuation = 1.0 - (distance / (light.raduis));
	if(attenuation < 0) attenuation = 0;
	if(attenuation > 1) attenuation = 1;
	
    vec3 radiance = vec3(1.0) * attenuation * light.intensity * intensitys;
	
    // Cook-Torrance BRDF direction
    float NDF = DistributionGGX(N, H, roughness_color);   
    float G   = GeometrySmith(N, V, L, roughness_color);      
    vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), F0);
	
    vec3 kd = mix(vec3(1.0) - F, vec3(0.0), metallic_color);
	vec3 diffuseBRDF = kd * albedo_color;
	vec3 specularBRDF = (F * NDF * G) / max(Epsilon, 4.0 * NdotL * 0.8);	       
	
	return (albedo_color / PI + diffuseBRDF + specularBRDF) * radiance * NdotL * light.color.rgb;
}

// Based on http://www.oscars.org/science-technology/sci-tech-projects/aces
vec3 ACESTonemap(vec3 color)
{
	mat3 m1 = mat3(
		0.59719, 0.07600, 0.02840,
		0.35458, 0.90834, 0.13383,
		0.04823, 0.01566, 0.83777
	);
	mat3 m2 = mat3(
		1.60475, -0.10208, -0.00327,
		-0.53108, 1.10813, -0.07276,
		-0.07367, -0.00605, 1.07602
	);
	vec3 v = m1 * color;
	vec3 a = v * (v + 0.0245786) - 0.000090537;
	vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
	return clamp(m2 * (a / b), 0.0, 1.0);
}

void main()
{
    vec4 texColor = texture(albedroMap, inUV); // if w is zero, no lighting calculation is required (background)
    if(texColor.w == 0.0)
    {
	    outColor = vec4(bloomState.Enabled ? texColor.rgb * bloomState.SkyboxMod : texColor.rgb, 1.0);
        return;
    }
    
    vec3 albedo = texColor.rgb;
    vec4 position = texture(positionsMap, inUV);
    vec4 normals = texture(normalsMap, inUV);
    vec4 materials = texture(materialsMap, inUV);
    vec4 shadowCoord = ( biasMat * lightSpace) * vec4(position.xyz, 1.0);
    vec3 ao = vec3(materials.z);

    float metallic = materials.x;
    float unclampedRoughness = materials.y;
	float roughness = max(unclampedRoughness, 0.04);
	float emission = materials.a;

	albedo = pow(albedo, vec3(2.2));
    vec3 V = normalize(sceneData.camPos.xyz - position.xyz);
	vec3 F0 =  vec3(0.04); 
	F0 = mix(F0, albedo, metallic);
	
    // Ambient Lighting (IBL)
	//--------------------------------------------

    vec3 ambient = vec3(0.0);
	if(sceneState.is_active == true)
	{
	    vec3 reflectionVec = reflect(-V, normals.xyz);

		ambient = CalcIBL(V , F0, reflectionVec, normals.xyz, albedo.rgb, ambient, metallic, unclampedRoughness, roughness, ao.r);
		ambient *= sceneState.iblScale;
	}

    vec3 Lo = vec3(0.0);
	// Direct Lighting
	//--------------------------------------------
	 if(dirLight.is_active == true)
	 {
		 Lo += CalcDirLight(V, normals.xyz, F0, albedo, metallic, roughness, position.xyz);
	 }

    // Point Lighting
	//--------------------------------------------
	for(int i = 0; i < numPointsLights; i++)
	{
        Lo += CalcPointLight(pointLights[i], V, normals.xyz, F0, albedo, metallic, roughness, position.xyz);
	}

    // Spot Lighting
	//--------------------------------------------
	for(int i = 0; i < numSpotLights; i++)
	{
		Lo += CalcSpotLight(spotLights[i], V, normals.xyz, F0, albedo, metallic, roughness, position.xyz);
	}

    // Final Shading
	//--------------------------------------------
	vec3 color = ambient + Lo;

    // Shadow Mapping
	//--------------------------------------------
	if(dirLight.is_active == true && dirLight.is_cast_shadows == true)
	{
		float shadow = 0.0;
		if(dirLight.is_use_soft_shadows == true)
		{
			shadow = filterPCF(shadowCoord / shadowCoord.w);
		}
		else
		{
			ivec2 texDim = textureSize(shadowMap, 0);
			shadow = textureProj(shadowCoord, texDim);
		}

		color *= shadow;
    }

	outColor.rgb = ACESTonemap(color * emission);
}