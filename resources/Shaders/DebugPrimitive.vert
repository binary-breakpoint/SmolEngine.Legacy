#version 460 core

layout(location = 0) in vec3 a_Position;

layout(push_constant) uniform DebugData
{
    mat4 model;
	mat4 model2;
	uint state;
};

layout (std140, binding = 27) uniform SceneBuffer
{
	mat4 projection;
	mat4 view;
	vec4 camPos;
	float nearClip;
    float farClip;
	vec2  pad1;
	mat4 skyBoxMatrix;

} sceneData;

void main()
{
	switch(state)
	{
		case 0:
        {
	        gl_Position = sceneData.projection * sceneData.view * model * vec4(a_Position, 1);
            break;
        }
        case 1:
        {
			if(gl_VertexIndex == 0)
			{
				gl_Position = sceneData.projection * sceneData.view * model * vec4(a_Position, 1);
			}
			else
			{
				gl_Position = sceneData.projection * sceneData.view * model2 * vec4(a_Position, 1);
			}

            break;
        }
	}
}