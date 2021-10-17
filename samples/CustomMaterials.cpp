#include "CustomMaterials.h"

#include <GraphicsCore.h>

using namespace SmolEngine;

class MagmaMaterial : public Material3D
{
public:

	void Create()
	{
		ShaderCreateInfo shaderCI{};
		shaderCI.IsSource = true;
		shaderCI.Stages[ShaderType::Vertex] = R"(#version 460 core

        layout(location = 0) in vec3 a_Position;
        layout(location = 1) in vec3 a_Normal;
        layout(location = 2) in vec3 a_Tangent;
        layout(location = 3) in vec2 a_UV;
        layout(location = 4) in ivec4 a_BoneIDs;
        layout(location = 5) in vec4 a_Weight;
        
        struct InstanceData
        {
        	uint matID;
        	uint isAnimated;
        	uint animOffset;
        	uint entityID;
        	mat4 model;
        };
        
        layout(std140, binding = 25) readonly buffer InstancesBuffer
        {   
        	InstanceData instances[];
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
        
        layout(std430, binding = 28) readonly buffer JointMatrices
        {
        	mat4 joints[];
        };
        
        layout(push_constant) uniform ConstantData
        {
        	uint dataOffset;
            float time;
        };
        
        
        layout (location = 0)  out vec3 v_FragPos;
        layout (location = 1)  out vec3 v_Normal;
        layout (location = 2)  out vec2 v_UV;
        layout (location = 3)  out float v_LinearDepth;
        layout (location = 4)  out float v_Time;
        
        void main()
        {
        	const uint instanceID = dataOffset + gl_InstanceIndex;
        	const mat4 model = instances[instanceID].model;
        	const uint animOffset = instances[instanceID].animOffset;
        	const bool isAnimated = bool(instances[instanceID].isAnimated);
        
        	mat4 skinMat = mat4(1.0);
        	if(isAnimated)
        	{
        		skinMat = 
        		a_Weight.x * joints[animOffset + uint(a_BoneIDs.x)] +
        		a_Weight.y * joints[animOffset + uint(a_BoneIDs.y)] +
        		a_Weight.z * joints[animOffset + uint(a_BoneIDs.z)] +
        		a_Weight.w * joints[animOffset + uint(a_BoneIDs.w)];
        	}
        
        	const mat4 modelSkin = model * skinMat;
        
        	v_FragPos = vec3(modelSkin *  vec4(a_Position, 1.0));
        	v_UV = a_UV;
        	v_LinearDepth = -(sceneData.view * vec4(v_FragPos, 1)).z;
            v_Time = time;
            v_Normal = mat3(transpose(inverse(modelSkin))) * a_Normal;	
        
        	gl_Position =  sceneData.projection * sceneData.view  * vec4(v_FragPos, 1.0);

            })";

	    shaderCI.Stages[ShaderType::Fragment] = R"(#version 460

        // Buffers
        // -----------------------------------------------------------------------------------------------------------------------
        
        // In
        layout (location = 0)  in vec3 v_FragPos;
        layout (location = 1)  in vec3 v_Normal;
        layout (location = 2)  in vec2 v_UV;
        layout (location = 3)  in float v_LinearDepth;
        layout (location = 4)  in float v_Time;
        
        // Out
        layout (location = 0) out vec4 out_color;
        layout (location = 1) out vec4 out_positions;
        layout (location = 2) out vec4 out_normals;
        layout (location = 3) out vec4 out_materials;
        
        vec3 fetchNormalMap() 
        {
           return normalize(v_Normal);
        }

        layout (std140, binding = 102) uniform MaterialBuffer
        {
           vec4 albedo;
           vec4 time;
        
        } material;

        // random2 function by Patricio Gonzalez
        vec2 random2( vec2 p ) {
            return fract(sin(vec2(dot(p,vec2(127.1,311.7)),dot(p,vec2(269.5,183.3))))*43758.5453);
        }
        
        // Value Noise by Inigo Quilez - iq/2013
        // https://www.shadertoy.com/view/lsf3WH
        float noise(vec2 st) {
            vec2 i = floor(st);
            vec2 f = fract(st);
        
            vec2 u = f*f*(3.0-2.0*f);
        
            return mix( mix( dot( random2(i + vec2(0.0,0.0) ), f - vec2(0.0,0.0) ), 
                             dot( random2(i + vec2(1.0,0.0) ), f - vec2(1.0,0.0) ), u.x),
                        mix( dot( random2(i + vec2(0.0,1.0) ), f - vec2(0.0,1.0) ), 
                             dot( random2(i + vec2(1.0,1.0) ), f - vec2(1.0,1.0) ), u.x), u.y);
        }
        
        vec3 magmaFunc(vec3 color, vec2 uv, float detail, float power,
                      float colorMul, float glowRate, bool animate, float noiseAmount)
        {
            vec3 rockColor = vec3(0.09 + abs(sin(v_Time * .75)) * .03, 0.02, .02);
            float minDistance = 1.;
            uv *= detail;
            
            vec2 cell = floor(uv);
            vec2 frac = fract(uv);
            
            for (int i = -1; i <= 1; i++) {
                for (int j = -1; j <= 1; j++) {
                	vec2 cellDir = vec2(float(i), float(j));
                    vec2 randPoint = random2(cell + cellDir);
                    randPoint += noise(uv) * noiseAmount;
                    randPoint = animate ? 0.5 + 0.5 * sin(v_Time * .35 + 6.2831 * randPoint) : randPoint;
                    minDistance = min(minDistance, length(cellDir + randPoint - frac));
                }
            }
            	
            float powAdd = sin(uv.x * 2. + v_Time * glowRate) + sin(uv.y * 2. + v_Time * glowRate);
        	vec3 outColor = vec3(color * pow(minDistance, power + powAdd * .95) * colorMul);
            outColor.rgb = mix(rockColor, outColor.rgb, minDistance);
            return outColor;
        }

        void main()
        {
            vec2 uv = v_UV;

            uv.x += v_Time * .01;
            vec4 fragColor = vec4(0.);
            fragColor.rgb += magmaFunc(vec3(1.5, .8, 0.), uv, 3.,  2.5, 1.15, 1.5, false, 1.5);
            fragColor.rgb += magmaFunc(vec3(1.5, 0., 0.), uv, 6., 3., .4, 1., false, 0.);
            fragColor.rgb += magmaFunc(vec3(1.5, .4, 0.), uv, 8., 4., .2, 1.9, true, 0.8);
            
        	vec3 N = fetchNormalMap(); 	
            out_color = fragColor;

            out_positions = vec4(v_FragPos, v_LinearDepth);
            out_normals = vec4(N, 1.0);
            out_materials = vec4(0.2, 0.7, 1.0, 1.0);
        })";

        MaterialCreateInfo ci{};
        ci.Name = "Magma Material";
        ci.PipelineCreateInfo.ShaderCreateInfo = shaderCI;
        ci.PipelineCreateInfo.VertexInputInfos = { GetVertexInputInfo() };

        Build(&ci);
	}

	virtual void OnPushConstant(const uint32_t& dataOffset) override
	{
        struct ps
        {
            uint32_t offset;
            float time;
        } push_constant;

        static float time = 0.0f;

        time += 0.01f;

        push_constant.offset = dataOffset;
        push_constant.time = time;

        SubmitPushConstant(ShaderType::Vertex, sizeof(ps), &push_constant);
	}

	virtual void OnDrawCommand(Ref<Mesh>& mesh, DrawPackage* command) override
	{
        DrawMeshIndexed(mesh, command->Instances);
	}
};

int main(int argc, char** argv)
{
	EditorCameraCreateInfo cameraCI = {};
	Camera* camera = new EditorCamera(&cameraCI);
	DebugLog* log = new DebugLog([&](const std::string&& msg, LogLevel level) { std::cout << msg << "\n"; });

	WindowCreateInfo windoInfo = {};
	{
		windoInfo.bFullscreen = false;
		windoInfo.bVSync = false;
		windoInfo.Height = 480;
		windoInfo.Width = 720;
		windoInfo.Title = "Custom Materials";
	}

	GraphicsContextCreateInfo info = {};
	info.ResourcesFolder = "../resources/";
	info.pWindowCI = &windoInfo;

	bool process = true;
	ClearInfo clearInfo = {};

	auto context = GraphicsContext::Create(&info);
	context->SetEventCallback([&](Event& e) { if (e.IsType(EventType::WINDOW_CLOSE)) { process = false; }  camera->OnEvent(e); });

	auto& [cube, cubeView] = MeshPool::GetSphere();
    auto cubeView2 = cube->CreateMeshView();

	{
		TextureCreateInfo textureCI = {};
		PBRCreateInfo materialCI = {};

		textureCI.FilePath = "Assets/materials/metal_1/Metal033_1K_Color.png";
		materialCI.SetTexture(PBRTexture::Albedo, &textureCI);

		textureCI.FilePath = "Assets/materials/metal_1/Metal033_1K_Normal.png";
		materialCI.SetTexture(PBRTexture::Normal, &textureCI);

		textureCI.FilePath = "Assets/materials/metal_1/Metal033_1K_Roughness.png";
		materialCI.SetTexture(PBRTexture::Roughness, &textureCI);

		textureCI.FilePath = "Assets/materials/metal_1/Metal033_1K_Metalness.png";
		materialCI.SetTexture(PBRTexture::Metallic, &textureCI);

		auto materialID = PBRFactory::AddMaterial(&materialCI, "metal");
        cubeView2->SetPBRHandle(materialID, cube->GetNodeIndex());

        PBRFactory::UpdateMaterials();
	}

	DirectionalLight dirLight = {};
	dirLight.IsActive = true;
	dirLight.Direction = glm::vec4(105.0f, 53.0f, 102.0f, 0);
	RendererDrawList::SubmitDirLight(&dirLight);

    RendererStorage::GetState().bDrawGrid = false;
    RendererStorage::GetState().Bloom.Enabled = true;

    Ref<MagmaMaterial> material = std::make_shared<MagmaMaterial>();
    material->Create();

    for (auto& mesh : cube->GetScene())
        cubeView->SetMaterial(material, mesh->GetNodeIndex());

    static float rotY = 0.0f;

	while (process)
	{
		context->ProcessEvents();
		float deltaTime = context->CalculateDeltaTime();

		if (context->IsWindowMinimized())
			continue;

		/*
			Calculates physics, update scripts, etc.
		*/

        rotY += 0.005f;
		camera->OnUpdate(deltaTime);

		RendererDrawList::BeginSubmit(camera->GetSceneViewProjection());
		RendererDrawList::SubmitMesh({ 0, 0, 0 }, { 0, rotY, 0 }, { 1, 1, 1 }, cube, cubeView);
        RendererDrawList::SubmitMesh({ 5, 0, 0 }, { 0, rotY, 0 }, { 1, 1, 1 }, cube, cubeView2);
		RendererDrawList::EndSubmit();

		context->BeginFrame(deltaTime);
		{
			ImGui::Begin("Panel");
			if (ImGui::Checkbox("Enable", &dirLight.IsActive))
			{
				RendererDrawList::SubmitDirLight(&dirLight);
			}

			if (ImGui::DragFloat4("Dir", glm::value_ptr(dirLight.Direction)))
			{
				RendererDrawList::SubmitDirLight(&dirLight);
			}
			ImGui::End();

			RendererDeferred::DrawFrame(&clearInfo);
		}
		context->SwapBuffers();
	}
}
