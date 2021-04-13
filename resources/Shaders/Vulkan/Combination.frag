#version 450
layout (location = 0) in vec2 inUV;
layout (binding = 0) uniform sampler2D colorSampler;
layout (binding = 1) uniform sampler2D bloomSampler;

layout (location = 0) out vec4 outFragColor;


layout(push_constant) uniform ConstantData
{
    float tone_exposure;
    float gamma;
};


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
    const float gamma = 2.2;
    vec3 hdrColor = texture(colorSampler, inUV).rgb;
    vec3 bloomColor = texture(bloomSampler, inUV).rgb;

    vec3 color = hdrColor + bloomColor; // additive blending

    outFragColor = vec4(color, 1.0);
}