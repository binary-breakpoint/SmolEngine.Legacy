#version 450 core

layout (location = 0) in vec2 inUV;

layout (binding = 0) uniform sampler2D albedro;
layout (binding = 1) uniform sampler2D position;
layout (binding = 2) uniform sampler2D normals;

layout (binding = 3) uniform samplerCube samplerIrradiance;
layout (binding = 4) uniform sampler2D samplerBRDFLUT;
layout (binding = 5) uniform samplerCube prefilteredMap;

layout(location = 0) out vec4 color;

layout(push_constant) uniform ConstantData
{
    vec3 camPos;
};

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
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

void main()
{
    vec3 albedo_color = texture(albedro, inUV).rgb;
    albedo_color = pow(albedo_color, vec3(2.2));

    vec3 ao = vec3(1.0);
    vec3 position = texture(position, inUV).rgb;
    vec3 normals = texture(normals, inUV).rgb;
    float metallic = 0.4;
    float roughness = 1.0;
    vec3 V = normalize(camPos - position); 

    vec3 F0 = mix(vec3(0.04), albedo_color, metallic); 
    vec3 ambient = CalcIBL(normals, V, F0, ao, albedo_color, metallic, roughness);

	color = vec4(ambient, 1.0);
}