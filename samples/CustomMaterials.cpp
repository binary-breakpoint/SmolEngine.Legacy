#include "CustomMaterials.h"

#include <GraphicsCore.h>

using namespace SmolEngine;

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

	auto& [cube, cubeView] = MeshPool::GetCube();

	{
		TextureCreateInfo textureCI = {};
		PBRMaterialCreateInfo materialCI = {};

		textureCI.FilePath = "Assets/materials/metal_1/Metal033_1K_Color.png";
		materialCI.SetTexture(PBRTexture::Albedo, &textureCI);

		textureCI.FilePath = "Assets/materials/metal_1/Metal033_1K_Normal.png";
		materialCI.SetTexture(PBRTexture::Normal, &textureCI);

		textureCI.FilePath = "Assets/materials/metal_1/Metal033_1K_Roughness.png";
		materialCI.SetTexture(PBRTexture::Roughness, &textureCI);

		textureCI.FilePath = "Assets/materials/metal_1/Metal033_1K_Metalness.png";
		materialCI.SetTexture(PBRTexture::Metallic, &textureCI);

		auto materialID = RendererStorage::GetDefaultMaterial()->AddMaterial(&materialCI, "metal");
		cubeView->SetDefaultMaterialHandle(materialID, cube->GetNodeIndex());

		RendererStorage::OnUpdateMaterials();
	}

	DirectionalLight dirLight = {};
	dirLight.IsActive = true;
	dirLight.Direction = glm::vec4(105.0f, 53.0f, 102.0f, 0);
	RendererDrawList::SubmitDirLight(&dirLight);

	while (process)
	{
		context->ProcessEvents();
		float deltaTime = context->CalculateDeltaTime();

		if (context->IsWindowMinimized())
			continue;

		/*
			Calculates physics, update scripts, etc.
		*/

		camera->OnUpdate(deltaTime);

		RendererDrawList::BeginSubmit(camera->GetSceneViewProjection());
		RendererDrawList::SubmitMesh({ 0, 0, 0 }, { 0, 0, 0 }, { 1, 1, 1 }, cube, cubeView);
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
