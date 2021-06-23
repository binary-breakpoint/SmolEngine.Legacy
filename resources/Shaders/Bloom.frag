#version 450

layout (binding = 0) uniform sampler2D colorSampler;
layout (binding = 1) uniform sampler2D emissionStrengthSampler;

layout (location = 0) in vec2 inUV;
layout (location = 0) out vec4 outColor;

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

// From the OpenGL Super bible
	const float weights[] = float[](0.0024499299678342,
									0.0043538453346397,
									0.0073599963704157,
									0.0118349786570722,
									0.0181026699707781,
									0.0263392293891488,
									0.0364543006660986,
									0.0479932050577658,
									0.0601029809166942,
									0.0715974486241365,
									0.0811305381519717,
									0.0874493212267511,
									0.0896631113333857,
									0.0874493212267511,
									0.0811305381519717,
									0.0715974486241365,
									0.0601029809166942,
									0.0479932050577658,
									0.0364543006660986,
									0.0263392293891488,
									0.0181026699707781,
									0.0118349786570722,
									0.0073599963704157,
									0.0043538453346397,
									0.0024499299678342);

void main(void)
{
	outColor = blur13(colorSampler, inUV, vec2(720, 480), vec2(1.0, 0.0));
}