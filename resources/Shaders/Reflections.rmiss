#version 460
#extension GL_EXT_ray_tracing : enable

struct hitPayload
{
	vec3 radiance;
	vec3 attenuation;
	int  done;
	vec3 rayOrigin;
	vec3 rayDir;
};

layout(location = 0) rayPayloadInEXT hitPayload prd;

void main()
{
	prd.radiance = vec3(0.3) * prd.attenuation;
	prd.done     = 1;
}