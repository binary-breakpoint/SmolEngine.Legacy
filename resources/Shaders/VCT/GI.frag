#version 460

layout (location = 0) in vec2 inUV;
layout(location = 0) out vec4 fragColor;

layout (binding = 0) uniform sampler2D albedroMap;
layout (binding = 1) uniform sampler2D positionsMap;
layout (binding = 2) uniform sampler2D normalsMap;
layout (binding = 3) uniform sampler2D materialsMap;

layout(binding = 4) uniform sampler2D shadowMap;
layout(binding = 5) uniform sampler3D voxelVisibility;

layout(binding = 6) uniform sampler3D voxelTex;
layout(binding = 7) uniform sampler3D voxelTexMipmap[6];

const float PI = 3.14159265f;
const float HALF_PI = 1.57079f;
const float EPSILON = 1e-30;
const float SQRT_3 = 1.73205080f;
const uint MAX_DIRECTIONAL_LIGHTS = 1;
const uint MAX_POINT_LIGHTS = 1;
const uint MAX_SPOT_LIGHTS = 1;

struct Attenuation
{
    float constant;
    float linear;
    float quadratic;
    float pad;
};

struct Light {

    vec3 ambient;
    float angleInnerCone;

    vec3 diffuse;
    float angleOuterCone;

    vec3 specular;
    float pad1;

    vec3 position;
    float pad2;

    vec3 direction;
    uint shadowingMethod;

    Attenuation attenuation;
};

layout(std140, binding = 304) uniform GIBuffer
{   
   mat4 lightViewProjection;

   vec3 cameraPosition;
   float lightBleedingReduction;

   vec2 exponents;
   float voxelScale;
   int volumeDimension;

   vec3 worldMaxPoint;
   float pad1;

   vec3 worldMinPoint;
   float pad2;
   
   Light directionalLight;
};

const float maxTracingDistanceGlobal = 1.0f;
const float bounceStrength = 1.0f;
const float aoFalloff = 725.0f;
const float aoAlpha = 0.01f;
const float samplingFactor = 0.5f;
const float coneShadowTolerance = 1.0f;
const float coneShadowAperture = 0.03f;
const uint mode = 0;


const vec3 diffuseConeDirections[] =
{
    vec3(0.0f, 1.0f, 0.0f),
    vec3(0.0f, 0.5f, 0.866025f),
    vec3(0.823639f, 0.5f, 0.267617f),
    vec3(0.509037f, 0.5f, -0.7006629f),
    vec3(-0.50937f, 0.5f, -0.7006629f),
    vec3(-0.823639f, 0.5f, 0.267617f)
};

const float diffuseConeWeights[] =
{
    PI / 4.0f,
    3.0f * PI / 20.0f,
    3.0f * PI / 20.0f,
    3.0f * PI / 20.0f,
    3.0f * PI / 20.0f,
    3.0f * PI / 20.0f,
};

vec3 WorldToVoxel(vec3 position)
{
    vec3 voxelPos = position - worldMinPoint;
    return voxelPos * voxelScale;
}

vec4 AnistropicSample(vec3 coord, vec3 weight, uvec3 face, float lod)
{
    // anisotropic volumes level
    float anisoLevel = max(lod - 1.0f, 0.0f);
    // directional sample
    vec4 anisoSample = weight.x * textureLod(voxelTexMipmap[face.x], coord, anisoLevel)
                     + weight.y * textureLod(voxelTexMipmap[face.y], coord, anisoLevel)
                     + weight.z * textureLod(voxelTexMipmap[face.z], coord, anisoLevel);
    // linearly interpolate on base level
    if(lod < 1.0f)
    {
        vec4 baseColor = texture(voxelTex, coord);
        anisoSample = mix(baseColor, anisoSample, clamp(lod, 0.0f, 1.0f));
    }

    return anisoSample;                    
}

bool IntersectRayWithWorldAABB(vec3 ro, vec3 rd, out float enter, out float leave)
{
    vec3 tempMin = (worldMinPoint - ro) / rd; 
    vec3 tempMax = (worldMaxPoint - ro) / rd;
    
    vec3 v3Max = max (tempMax, tempMin);
    vec3 v3Min = min (tempMax, tempMin);
    
    leave = min (v3Max.x, min (v3Max.y, v3Max.z));
    enter = max (max (v3Min.x, 0.0), max (v3Min.y, v3Min.z));    
    
    return leave > enter;
}

vec4 TraceCone(vec3 position, vec3 normal, vec3 direction, float aperture, bool traceOcclusion)
{
    uvec3 visibleFace;
    visibleFace.x = (direction.x < 0.0) ? 0 : 1;
    visibleFace.y = (direction.y < 0.0) ? 2 : 3;
    visibleFace.z = (direction.z < 0.0) ? 4 : 5;
    traceOcclusion = traceOcclusion && aoAlpha < 1.0f;
    // world space grid voxel size
    float voxelWorldSize = 2.0 /  (voxelScale * volumeDimension);
    // weight per axis for aniso sampling
    vec3 weight = direction * direction;
    // move further to avoid self collision
    float dst = voxelWorldSize;
    vec3 startPosition = position + normal * dst;
    // final results
    vec4 coneSample = vec4(0.0f);
    float occlusion = 0.0f;
    float maxDistance = maxTracingDistanceGlobal * (1.0f / voxelScale);
    float falloff = 0.5f * aoFalloff * voxelScale;
    // out of boundaries check
    float enter = 0.0; float leave = 0.0;

    if(!IntersectRayWithWorldAABB(position, direction, enter, leave))
    {
        coneSample.a = 1.0f;
    }

    while(coneSample.a < 1.0f && dst <= maxDistance)
    {
        vec3 conePosition = startPosition + direction * dst;
        // cone expansion and respective mip level based on diameter
        float diameter = 2.0f * aperture * dst;
        float mipLevel = log2(diameter / voxelWorldSize);
        // convert position to texture coord
        vec3 coord = WorldToVoxel(conePosition);
        // get directional sample from anisotropic representation
        vec4 anisoSample = AnistropicSample(coord, weight, visibleFace, mipLevel);
        // front to back composition
        coneSample += (1.0f - coneSample.a) * anisoSample;
        // ambient occlusion
        if(traceOcclusion && occlusion < 1.0)
        {
            occlusion += ((1.0f - occlusion) * anisoSample.a) / (1.0f + falloff * diameter);
        }
        // move further into volume
        dst += diameter * samplingFactor;
    }

    return vec4(coneSample.rgb, occlusion);
}

float TraceShadowCone(vec3 position, vec3 direction, float aperture, float maxTracingDistance) 
{
    bool hardShadows = false;

    if(coneShadowTolerance == 1.0f) { hardShadows = true; }

    // directional dominat axis
    uvec3 visibleFace;
    visibleFace.x = (direction.x < 0.0) ? 0 : 1;
    visibleFace.y = (direction.y < 0.0) ? 2 : 3;
    visibleFace.z = (direction.z < 0.0) ? 4 : 5;
    // world space grid size
    float voxelWorldSize = 2.0 /  (voxelScale * volumeDimension);
    // weight per axis for aniso sampling
    vec3 weight = direction * direction;
    // move further to avoid self collision
    float dst = voxelWorldSize;
    vec3 startPosition = position + direction * dst;
    // control vars
    float mipMaxLevel = log2(volumeDimension) - 1.0f;
    // final results
    float visibility = 0.0f;
    float k = exp2(7.0f * coneShadowTolerance);
    // cone will only trace the needed distance
    float maxDistance = maxTracingDistance;
    // out of boundaries check
    float enter = 0.0; float leave = 0.0;

    if(!IntersectRayWithWorldAABB(position, direction, enter, leave))
    {
        visibility = 1.0f;
    }
    
    while(visibility < 1.0f && dst <= maxDistance)
    {
        vec3 conePosition = startPosition + direction * dst;
        float diameter = 2.0f * aperture * dst;
        float mipLevel = log2(diameter / voxelWorldSize);
        // convert position to texture coord
        vec3 coord = WorldToVoxel(conePosition);
        // get directional sample from anisotropic representation
        vec4 anisoSample = AnistropicSample(coord, weight, visibleFace, mipLevel);

        // hard shadows exit as soon cone hits something
        if(hardShadows && anisoSample.a > EPSILON) { return 0.0f; }  
        // accumulate
        visibility += (1.0f - visibility) * anisoSample.a * k;
        // move further into volume
        dst += diameter * samplingFactor;
    }

    return 1.0f - visibility;
}

float linstep(float low, float high, float value)
{
    return clamp((value - low) / (high - low), 0.0f, 1.0f);
}  

float ReduceLightBleeding(float pMax, float Amount)  
{  
    return linstep(Amount, 1, pMax);  
} 

vec2 WarpDepth(float depth)
{
    depth = 2.0f * depth - 1.0f;
    float pos = exp(exponents.x * depth);
    float neg = -exp(-exponents.y * depth);
    return vec2(pos, neg);
}

float Chebyshev(vec2 moments, float mean, float minVariance)
{
    if(mean <= moments.x)
    {
        return 1.0f;
    }
    else
    {
        float variance = moments.y - (moments.x * moments.x);
        variance = max(variance, minVariance);
        float d = mean - moments.x;
        float lit = variance / (variance + (d * d));
        return ReduceLightBleeding(lit, lightBleedingReduction);
    }
}

float Visibility(vec3 position)
{
    vec4 lsPos = lightViewProjection * vec4(position, 1.0f);
    // avoid arithmetic error
    if(lsPos.w == 0.0f) return 1.0f;
    // transform to ndc-space
    lsPos /= lsPos.w;
    // querry visibility
    vec4 moments = texture(shadowMap, lsPos.xy);
    // move to avoid acne
    vec2 wDepth = WarpDepth(lsPos.z - 0.0001f);
    // derivative of warping at depth
    vec2 depthScale = 0.0001f * exponents * wDepth;
    vec2 minVariance = depthScale * depthScale;
    // evsm mode 4 compares negative and positive
    float positive = Chebyshev(moments.xz, wDepth.x, minVariance.x);
    float negative = Chebyshev(moments.yw, wDepth.y, minVariance.y);
    // shadowing value
    return min(positive, negative);
}

vec3 Ambient(Light light, vec3 albedo)
{
    return max(albedo * light.ambient, 0.0f);
}

vec3 BRDF(Light light, vec3 N, vec3 X, vec3 ka, vec4 ks)
{
    // common variables
    vec3 L = light.direction;
    vec3 V = normalize(cameraPosition - X);
    vec3 H = normalize(V + L);
    // compute dot procuts
    float dotNL = max(dot(N, L), 0.0f);
    float dotNH = max(dot(N, H), 0.0f);
    float dotLH = max(dot(L, H), 0.0f);
    // decode specular power
    float spec = exp2(11.0f * ks.a + 1.0f);
    // emulate fresnel effect
    vec3 fresnel = ks.rgb + (1.0f - ks.rgb) * pow(1.0f - dotLH, 5.0f);
    // specular factor
    float blinnPhong = pow(dotNH, spec);
    // energy conservation, aprox normalization factor
    blinnPhong *= spec * 0.0397f + 0.3183f;
    // specular term
    vec3 specular = ks.rgb * light.specular * blinnPhong * fresnel;
    // diffuse term
    vec3 diffuse = ka.rgb * light.diffuse;
    // return composition
    return (diffuse + specular) * dotNL;
}

vec3 CalculateDirectional(Light light, vec3 normal, vec3 position, vec3 albedo, vec4 specular)
{
    float visibility = 1.0f;

    if(light.shadowingMethod == 1)
    {
        visibility = Visibility(position);
    }
    else if(light.shadowingMethod == 2)
    {
        visibility = max(0.0f, TraceShadowCone(position, light.direction, coneShadowAperture, 1.0f / voxelScale));
    }
    else if(light.shadowingMethod == 3)
    {
        vec3 voxelPos = WorldToVoxel(position);  
        visibility = max(0.0f, texture(voxelVisibility, voxelPos).a);
    }

    if(visibility <= 0.0f) return vec3(0.0f);  

    return BRDF(light, normal, position, albedo, specular) * visibility;
}

vec3 CalculatePoint(Light light, vec3 normal, vec3 position, vec3 albedo, vec4 specular)
{
    light.direction = light.position - position;
    float d = length(light.direction);
    light.direction = normalize(light.direction);
    float falloff = 1.0f / (light.attenuation.constant + light.attenuation.linear * d
                    + light.attenuation.quadratic * d * d + 1.0f);

    if(falloff <= 0.0f) return vec3(0.0f);

    float visibility = 1.0f;

    if(light.shadowingMethod == 2)
    {
        visibility = max(0.0f, TraceShadowCone(position, light.direction, coneShadowAperture, d));
    }
    else if(light.shadowingMethod == 3)
    {
        vec3 voxelPos = WorldToVoxel(position);  
        visibility = max(0.0f, texture(voxelVisibility, voxelPos).a);
    } 

    if(visibility <= 0.0f) return vec3(0.0f);  

    return BRDF(light, normal, position, albedo, specular) * falloff * visibility;
}

vec3 CalculateSpot(Light light, vec3 normal, vec3 position, vec3 albedo, vec4 specular)
{
    vec3 spotDirection = light.direction;
    light.direction = normalize(light.position - position);
    float cosAngle = dot(-light.direction, spotDirection);

    // outside the cone
    if(cosAngle < light.angleOuterCone) { return vec3(0.0f); }

    // assuming they are passed as cos(angle)
    float innerMinusOuter = light.angleInnerCone - light.angleOuterCone;
    // spot light factor for smooth transition
    float spotMark = (cosAngle - light.angleOuterCone) / innerMinusOuter;
    float spotFalloff = smoothstep(0.0f, 1.0f, spotMark);

    if(spotFalloff <= 0.0f) return vec3(0.0f);   

    float dst = distance(light.position, position);
    float falloff = 1.0f / (light.attenuation.constant + light.attenuation.linear * dst
                    + light.attenuation.quadratic * dst * dst + 1.0f);   

    if(falloff <= 0.0f) return vec3(0.0f);

    float visibility = 1.0f;


    if(light.shadowingMethod == 2)
    {
        visibility = max(0.0f, TraceShadowCone(position, light.direction, coneShadowAperture, dst));
    }
    else if(light.shadowingMethod == 3)
    {
        vec3 voxelPos = WorldToVoxel(position);  
        visibility = max(0.0f, texture(voxelVisibility, voxelPos).a);
    } 

    if(visibility <= 0.0f) return vec3(0.0f); 

    return BRDF(light, normal, position, albedo, specular) * falloff * spotFalloff * visibility;
}

vec3 PositionFromDepth()
{
    vec3 position = texture(positionsMap, inUV).xyz;
    return position;
}

vec3 CalculateDirectLighting(vec3 position, vec3 normal, vec3 albedo, vec4 specular)
{
    // calculate directional lighting
    vec3 directLighting = vec3(0.0f);

    // calculate lighting for directional lights
        directLighting += CalculateDirectional(directionalLight, normal, position, 
                                         albedo, specular);
        directLighting += Ambient(directionalLight, albedo);

    return directLighting;
}

vec4 CalculateIndirectLighting(vec3 position, vec3 normal, vec3 albedo, vec4 specular, bool ambientOcclusion)
{
    vec4 specularTrace = vec4(0.0f);
    vec4 diffuseTrace = vec4(0.0f);
    vec3 coneDirection = vec3(0.0f);

    // component greater than zero
    if(any(greaterThan(specular.rgb, specularTrace.rgb)))
    {
        vec3 viewDirection = normalize(cameraPosition - position);
        vec3 coneDirection = reflect(-viewDirection, normal);
        coneDirection = normalize(coneDirection);
        // specular cone setup, minimum of 1 grad, fewer can severly slow down performance
        float aperture = clamp(tan(HALF_PI * (1.0f - specular.a)), 0.0174533f, PI);
        specularTrace = TraceCone(position, normal, coneDirection, aperture, false);
        specularTrace.rgb *= specular.rgb;
    }

    // component greater than zero
    if(any(greaterThan(albedo, diffuseTrace.rgb)))
    {
        // diffuse cone setup
        const float aperture = 0.57735f;
        vec3 guide = vec3(0.0f, 1.0f, 0.0f);

        if (abs(dot(normal,guide)) == 1.0f)
        {
            guide = vec3(0.0f, 0.0f, 1.0f);
        }

        // Find a tangent and a bitangent
        vec3 right = normalize(guide - dot(normal, guide) * normal);
        vec3 up = cross(right, normal);

        for(int i = 0; i < 6; i++)
        {
            coneDirection = normal;
            coneDirection += diffuseConeDirections[i].x * right + diffuseConeDirections[i].z * up;
            coneDirection = normalize(coneDirection);
            // cumulative result
            diffuseTrace += TraceCone(position, normal, coneDirection, aperture, ambientOcclusion) * diffuseConeWeights[i];
        }

        diffuseTrace.rgb *= albedo;
    }

    vec3 result = bounceStrength * (diffuseTrace.rgb + specularTrace.rgb);

    return vec4(result, ambientOcclusion ? clamp(1.0f - diffuseTrace.a + aoAlpha, 0.0f, 1.0f) : 1.0f);
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
    // world-space position
    vec3 position = PositionFromDepth();
    // world-space normal
    vec3 normal = normalize(texture(normalsMap, inUV).xyz);
    // xyz = fragment specular, w = shininess
    vec4 specular = vec4(1, 1, 1, 1);
    // fragment albedo

    vec4 baseColor = texture(albedroMap, inUV); // if w is zero, no lighting calculation is required (background)
    if(baseColor.w == 0.0)
    {
	    fragColor = vec4( baseColor.rgb, 1.0);
        return;
    }

    // convert to linear space
    vec3 albedo = pow(baseColor.rgb, vec3(2.2f));

    // fragment emissiviness
    vec4 materials = texture(materialsMap, inUV);
    vec3 emissive = vec3(0);
    
    // lighting cumulatives
    vec3 directLighting = vec3(1.0f);
    vec4 indirectLighting = vec4(1.0f);
    vec3 compositeLighting = vec3(1.0f);

    if(mode == 0)   // direct + indirect + ao
    {
        indirectLighting = CalculateIndirectLighting(position, normal, baseColor.rgb, specular, true);
        directLighting = CalculateDirectLighting(position, normal, albedo, specular);
    }
    else if(mode == 1)  // direct + indirect
    {
        indirectLighting = CalculateIndirectLighting(position, normal, baseColor.rgb, specular, false);
        directLighting = CalculateDirectLighting(position, normal, albedo, specular);
    }
    else if(mode == 2) // direct only
    {
        indirectLighting = vec4(0.0f, 0.0f, 0.0f, 1.0f);
        directLighting = CalculateDirectLighting(position, normal, albedo, specular);
    }
    else if(mode == 3) // indirect only
    {
        directLighting = vec3(0.0f);
        baseColor.rgb = specular.rgb = vec3(1.0f);
        indirectLighting = CalculateIndirectLighting(position, normal, baseColor.rgb, specular, false);
    }
    else if(mode == 4) // ambient occlusion only
    {
        directLighting = vec3(0.0f);
        specular = vec4(0.0f);
        indirectLighting = CalculateIndirectLighting(position, normal, baseColor.rgb, specular, true);
        indirectLighting.rgb = vec3(1.0f);
    }

    // final composite lighting (direct + indirect) * ambient occlusion
    compositeLighting = (directLighting + indirectLighting.rgb) * indirectLighting.a;
    compositeLighting += emissive;

    fragColor = vec4(ACESTonemap(compositeLighting), 1);
}