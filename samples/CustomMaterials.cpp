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
	info.eMSAASamples = MSAASamples::SAMPLE_COUNT_1;
	info.ResourcesFolder = "../resources/";
	info.pWindowCI = &windoInfo;

	bool process = true;
	ClearInfo clearInfo = {};

	auto context = GraphicsContext::Create(&info);
	context->SetEventCallback([&](Event& e) { if (e.IsType(EventType::WINDOW_CLOSE)) { process = false; }  camera->OnEvent(e); });

	auto cube = MeshPool::GetCube();
	auto drawList = new RendererDrawList();
	auto storage = new RendererStorage();
	storage->Build();

	Ref<PBRHandle> pbr_material = nullptr;
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

		pbr_material = storage->GetDefaultMaterial()->AddMaterial(&materialCI, "wood");

		storage->OnUpdateMaterials();
	}

	DirectionalLight dirLight = {};
	dirLight.IsActive = true;
	dirLight.Direction = glm::vec4(105.0f, 53.0f, 102.0f, 0);
	drawList->SubmitDirLight(&dirLight);

	SceneViewProjection viewProj = SceneViewProjection(camera);

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
		viewProj.Update(camera);

		drawList->BeginSubmit(&viewProj);
		drawList->SubmitMesh({ 0, 0, 0 }, { 0, 0, 0 }, { 1, 1, 1 }, cube, pbr_material);
		drawList->EndSubmit();

		context->BeginFrame(deltaTime);
		{
			ImGui::Begin("Panel");
			if (ImGui::Checkbox("Enable", &dirLight.IsActive))
			{
				drawList->SubmitDirLight(&dirLight);
			}

			if (ImGui::DragFloat4("Dir", glm::value_ptr(dirLight.Direction)))
			{
				drawList->SubmitDirLight(&dirLight);
			}
			ImGui::End();

			RendererDeferred::DrawFrame(&clearInfo, storage, drawList);
		}
		context->SwapBuffers();
	}
}
