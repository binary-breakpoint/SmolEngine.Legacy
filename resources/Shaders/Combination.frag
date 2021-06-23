#version 460

layout (location = 0) in vec2 inUV;

layout (binding = 0) uniform sampler2D colorSampler;
layout (binding = 1) uniform sampler2D bloomSampler;

layout (location = 0) out vec4 outFragColor;

layout(push_constant) uniform ConstantData
{
    uint ppState;
    uint vertical_blur; 
};

layout(std140, binding = 34) uniform BloomProperties
{   
	float exposure;
	float threshold;
	float scale;
	float strength;
} bloomState;

vec4 blur13(sampler2D image, vec2 uv, vec2 resolution, vec2 direction)
{
  vec4 color = vec4(0.0);
  vec2 off1 = vec2(1.411764705882353) * direction;
  vec2 off2 = vec2(3.2941176470588234) * direction;
  vec2 off3 = vec2(5.176470588235294) * direction;
  color += texture(image, uv) * 0.1964825501511404;
  color += texture(image, uv + (off1 / resolution)) * 0.2969069646728344 * bloomState.scale;
  color += texture(image, uv - (off1 / resolution)) * 0.2969069646728344 * bloomState.scale;
  color += texture(image, uv + (off2 / resolution)) * 0.09447039785044732 * bloomState.scale;
  color += texture(image, uv - (off2 / resolution)) * 0.09447039785044732 * bloomState.scale;
  color += texture(image, uv + (off3 / resolution)) * 0.010381362401148057 * bloomState.scale;
  color += texture(image, uv - (off3 / resolution)) * 0.010381362401148057 * bloomState.scale;
  return color;
}

void main() 
{
    vec4 color = texture(colorSampler, inUV);
    switch(ppState)
    {
        case 1:
        {
            vec4 bloom = vec4(0.0);
            if(vertical_blur == 1)
            {
                // vertical blur
                bloom = blur13(bloomSampler, inUV, vec2(720, 480), vec2(0.0, 1.0));
            }
            else
            {
                bloom = texture(bloomSampler, inUV);
            }

            
            color = vec4(color.rgb + bloom.rgb, 1.0);
            break;
        }
    }

    outFragColor = color;
}