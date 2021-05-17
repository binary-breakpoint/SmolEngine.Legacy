#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec3 a_Tangent;
layout(location = 3) in vec2 a_UV;
layout(location = 4) in ivec4 a_BoneIDs;
layout(location = 5) in vec4 a_Weight;

layout(std140, binding = 1) uniform ModelData
{   
	mat4 models[1000];
};

layout(std140, binding = 2) uniform CameraData
{   
	mat4 projection;
	mat4 view;
};

layout(location = 0) out vec4 v_color;
layout(location = 1) out vec3 v_pos;
layout(location = 2) out vec3 v_normals;

float rand(vec2 co)
{
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

void main()
{
	const mat4 model = models[gl_InstanceIndex];
	const vec2 co = vec2(gl_VertexIndex, gl_InstanceIndex);
	const float c = rand(co);

	v_color = vec4(c, c, c, 1);
	v_pos = vec3(model * vec4(a_Position, 1.0));
	v_normals = mat3(model) * a_Normal;

    gl_Position =  projection * view * vec4(v_pos, 1.0);
}