#version 450 core
layout (location = 0) in vec2 inUV;
layout(location = 0) out vec4 o_Color;

layout (binding = 0) uniform sampler2D u_Texture;
layout (binding = 1) uniform sampler2D u_DepthTexture;

const float GOLDEN_ANGLE = 2.39996323;
const float MAX_BLUR_SIZE = 20.0;
const float RAD_SCALE = 1.0; // Smaller = nicer blur, larger = faster
const float uFar = 1000.0;

float LinearizeDepth(float d, float zNear, float zFar)
{
	return zNear * zFar / (zFar + d * (zNear - zFar));
}

float GetBlurSize(float depth, float focusPoint, float focusScale)
{
	float coc = clamp((1.0 / focusPoint - 1.0 / depth) * focusScale, -1.0, 1.0);
	return abs(coc) * MAX_BLUR_SIZE;
}

vec3 DepthOfField(vec2 texCoord, float focusPoint, float focusScale, vec2 texelSize)
{
	float centerDepth = LinearizeDepth(texture(u_DepthTexture, texCoord).w, 0.1, uFar);// *uFar;
	centerDepth = texture(u_DepthTexture, texCoord).w * uFar;
	float centerSize = GetBlurSize(centerDepth, focusPoint, focusScale);
	vec3 color = texture(u_Texture, texCoord).rgb;
	float tot = 1.0;
	float radius = RAD_SCALE;
	for (float ang = 0.0; radius < MAX_BLUR_SIZE; ang += GOLDEN_ANGLE)
	{
		vec2 tc = texCoord + vec2(cos(ang), sin(ang)) * texelSize * radius;
		vec3 sampleColor = texture(u_Texture, tc).rgb;
		float sampleDepth = texture(u_DepthTexture, tc).w * uFar;
		float sampleSize = GetBlurSize(sampleDepth, focusPoint, focusScale);
		if (sampleDepth > centerDepth)
			sampleSize = clamp(sampleSize, 0.0, centerSize * 2.0);
		float m = smoothstep(radius - 0.5, radius + 0.5, sampleSize);
		color += mix(color / tot, sampleColor, m);
		tot += 1.0;
		radius += RAD_SCALE / radius;
	}
	return color /= tot;
}

void main()
{
	ivec2 texSize = textureSize(u_Texture, 0);
	vec2 fTexSize = vec2(float(texSize.x), float(texSize.y));
	float centerDepth = LinearizeDepth(texture(u_DepthTexture, vec2(0.5f)).w, 0.1, uFar);// *uFar;

	float focusPoint = 0.2f;
	float focusScale = 0.1;
	vec3 color = DepthOfField(inUV, focusPoint, focusScale, 1.0f / fTexSize);
	o_Color = vec4(color, 1.0);

}