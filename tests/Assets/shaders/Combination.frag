#version 450 core

layout (location = 0) in vec2 inUV;

layout (binding = 0) uniform sampler2D albedro;
layout (binding = 1) uniform sampler2D position;
layout (binding = 2) uniform sampler2D normals;

layout (binding = 3) uniform samplerCube samplerIrradiance;
layout (binding = 4) uniform sampler2D samplerBRDFLUT;
layout (binding = 5) uniform samplerCube prefilteredMap;

layout(location = 0) out vec4 color;

layout(std140, binding = 10) uniform CameraData
{   
	mat4 projection;
	mat4 view;
	vec4 camPos;
};

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

vec3 CalcIBL(vec3 N, vec3 V, vec3 F0, vec3 ao, vec3 albedo_color, float metallic_color, float roughness_color)
{
	float cosLo = max(dot(N, -V), 0.0);
	vec3 ReflDirectionWS = reflect(-V, N);
    vec3 F = fresnelSchlickRoughness(cosLo, F0, roughness_color);
	
    vec3 kD = (1.0 - F) * (1.0 - metallic_color);
    
    vec3 irradiance = texture(samplerIrradiance, -N).rgb;
	vec3 specularBRDF = texture(samplerBRDFLUT, vec2(cosLo, 1.0 - roughness_color)).rgb;
    vec3 diffuseIBL = irradiance * ( kD * albedo_color);
    
	int specularTextureLevels = textureQueryLevels(prefilteredMap);
	vec3 specularIrradiance =  textureLod(prefilteredMap, -ReflDirectionWS, roughness_color * specularTextureLevels).rgb;
    vec3 specularIBL = specularIrradiance * (F0 * specularBRDF.x + specularBRDF.y);
	specularIBL = specularIBL / 2.5;
    
	return (diffuseIBL + specularIBL) * ao;
}

vec3 CalcDirLight(vec3 V, vec3 N, vec3 F0, vec3 pos, vec3 albedo_color, float metallic_color, float roughness_color)
{
    float intensity = 1;
    vec3 dir = vec3(105.0, 53.0, 102.0);

	vec3 L = normalize(dir - pos);
    vec3 H = normalize(V + L);
	float NdotL = max(dot(N, L), 0.0);       
	
    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness_color);   
    float G   = GeometrySmith(N, V, L, roughness_color);      
    vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), F0);
	
    vec3 kd = mix(vec3(1.0) - F, vec3(0.0), metallic_color);
	vec3 diffuseBRDF = kd * albedo_color;
	vec3 specularBRDF = (F * NDF * G) / max(Epsilon, 4.0 * NdotL * 0.8);
	
	return ((albedo_color / PI + diffuseBRDF + specularBRDF) * vec3(1.0) * NdotL) * intensity;
}

// From http://filmicgames.com/archives/75
vec3 Uncharted2Tonemap(vec3 x)
{
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

void main()
{
    vec3 albedo_color = texture(albedro, inUV).rgb;
    albedo_color = pow(albedo_color, vec3(2.2));

    vec4 position = texture(position, inUV);
    vec4 normals = texture(normals, inUV);
    float metallic = position.w;
    float roughness = normals.w;
    vec3 V = normalize(camPos.xyz - position.xyz);
    vec3 ao = vec3(1.0);

    vec3 F0 = mix(vec3(0.04), albedo_color, metallic); 
    vec3 ambient = CalcIBL(normals.xyz, V, F0, ao, albedo_color, metallic, roughness);
    //vec3 dirLight = CalcDirLight(V, normals.xyz, F0, position.xyz, albedo_color, metallic, roughness);

    vec3 result = ambient;

	color = vec4(result, 1.0);
}