#version 450
#extension GL_ARB_shader_image_load_store : require

layout(location = 0) out vec4 albedo;

layout(binding = 0, rgba8) uniform readonly image3D voxelRadiance;

layout (std140, binding = 202) uniform StorageBlock
{
    uint volumeDimension;
    float voxelSize;
    vec2 pad1;

	vec4 colorChannels;
    vec4 frustumPlanes[6];

    vec3 worldMinPoint;
    float pad2;

    mat4 modelViewProjection;
};

void main()
{
	float volumeDimensionF = float(volumeDimension);

	vec3 position = vec3
	(
		gl_VertexIndex % volumeDimension,
		(gl_VertexIndex / volumeDimension) % volumeDimension,
		gl_VertexIndex / (volumeDimension * volumeDimension)
	);

	ivec3 texPos = ivec3(position);
	albedo = imageLoad(voxelRadiance, texPos);

	uvec4 channels = uvec4(floor(colorChannels));

	albedo = vec4(albedo.rgb * channels.rgb, albedo.a);
	// if no color channel is enabled but alpha is one, show alpha as rgb
	if(all(equal(channels.rgb, uvec3(0))) && channels.a > 0) 
	{
		albedo = vec4(albedo.a);
	}

	gl_Position = vec4(position, 1.0f);
}