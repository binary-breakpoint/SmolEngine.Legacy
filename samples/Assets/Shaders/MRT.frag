#version 450 core

layout(location = 0) in vec4 v_color;
layout(location = 1) in vec3 v_pos;
layout(location = 2) in vec3 v_normals;

layout(location = 0) out vec4 albedro;
layout(location = 1) out vec4 position;
layout(location = 2) out vec4 normals;


void main()
{
	albedro = v_color;
	position = vec4(v_pos, 1.0);
	normals = vec4(v_normals, 1.0);
}