#include "CustomMaterials.h"

#include <GraphicsCore.h>

using namespace SmolEngine;

class MagmaMaterial : public Material3D
{
public:

	void Create()
	{
		ShaderCreateInfo shaderCI{};
		shaderCI.Stages[ShaderType::Vertex] = "Assets/shaders/Magma.vert";
	    shaderCI.Stages[ShaderType::Fragment] = "Assets/shaders/Magma.frag";;

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

	auto context = GraphicsContext::Create(&info);
	context->SetEventCallback([&](Event& e) 
     { 
        camera->OnEvent(e);
     });

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
    ClearInfo clearInfo = {};

	while (context->IsOpen())
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
