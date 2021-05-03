#include "CSharpBinding.h"

#include <Common/Mesh.h>
#include <Common/Input.h>
#include <GraphicsContext.h>
#include <MaterialLibrary.h>
#include <Renderer.h>

#include <ImGUI/ImGuiExtension.h>

using namespace Frostium;

GraphicsContext* context = nullptr;

int main(int argc, char** argv)
{
	WindowCreateInfo windoInfo = {};
	{
		windoInfo.bFullscreen = false;
		windoInfo.bVSync = false;
		windoInfo.Height = 480;
		windoInfo.Width = 720;
		windoInfo.Title = "Frostium Example";
	}

	EditorCamera* camera = nullptr;
	{
		EditorCameraCreateInfo cameraCI = {};
		camera = new EditorCamera(&cameraCI);
	}

	GraphicsContextInitInfo info = {};
	{
		info.Flags = Features_Renderer_3D_Flags | Features_ImGui_Flags;
		info.ResourcesFolderPath = "../resources/";
		info.pWindowCI = &windoInfo;
		info.pDefaultCamera = camera;
		info.eShadowMapSize = ShadowMapSize::SIZE_16;
	}

	bool process = true;
	ClearInfo clearInfo = {};
	context = new GraphicsContext(&info);
	context->SetEventCallback([&](Event& e)
		{
			if (e.IsType(EventType::WINDOW_CLOSE))
				process = false;
		});

	static glm::vec3 lightDir = glm::vec3(105.0f, 53.0f, 102.0f);
	// Load assets
	Mesh plane = {};
	Mesh knight2 = {};
	Mesh cube = {};
	Mesh::Create("Assets/plane.gltf", &plane);
	Mesh::Create("Assets/CesiumMan.gltf", &knight2);
	Mesh::Create("Assets/cube.gltf", &cube);
	MaterialCreateInfo materialCI = {};
	materialCI.SetTexture(MaterialTexture::Albedro, "Assets/materials/stone/Tiles087_1K_Color.png");
	materialCI.SetTexture(MaterialTexture::Normal, "Assets/materials/stone/Tiles087_1K_Normal.png");
	materialCI.SetTexture(MaterialTexture::Roughness, "Assets/materials/stone/Tiles087_1K_Roughness.png");
	materialCI.SetTexture(MaterialTexture::AO, "Assets/materials/stone/Tiles087_1K_AmbientOcclusion.png");
	materialCI.SetMetalness(0.1f);
	uint32_t stoneMat = MaterialLibrary::GetSinglenton()->Add(&materialCI);

	materialCI = {};
	materialCI.SetTexture(MaterialTexture::Albedro, "Assets/materials/plane/Metal021_2K_Color.png");
	materialCI.SetTexture(MaterialTexture::Normal, "Assets/materials/plane/Metal021_2K_Normal.png");
	materialCI.SetTexture(MaterialTexture::Roughness, "Assets/materials/plane/Metal021_2K_Roughness.png");
	materialCI.SetTexture(MaterialTexture::Metallic, "Assets/materials/plane/Metal021_2K_Metalness.png");
	uint32_t planeMat = MaterialLibrary::GetSinglenton()->Add(&materialCI);

	Renderer::UpdateMaterials();

	plane.SetMaterialID(stoneMat, true);
	AnimationProperties* defaultProp = plane.GetAnimationProperties(0);
	AnimationProperties* defaultProp2 = knight2.GetAnimationProperties(0);

	defaultProp2->SetActive(true);
	defaultProp2->SetSpeed(5.0f);

	bool playAnim = false;
	bool reset = false;

	glm::vec3 rot = glm::vec3(-90.0f, 0.0f, 0.0f);

	while (process)
	{
		context->ProcessEvents();
		DeltaTime deltaTime = context->CalculateDeltaTime();

		if (context->IsWindowMinimized())
			continue;

		context->BeginFrame(deltaTime);
		{
			ImGui::Begin("Debug Window");
			{
				if (ImGui::Checkbox("Play", &playAnim))
				{
					defaultProp->SetActive(true);
					reset = false;
				}

				if (ImGui::Checkbox("Reset", &reset))
				{
					plane.ResetAnimation(0);
					playAnim = false;
				}

				if (!playAnim && defaultProp->IsActive())
				{
					defaultProp->SetActive(false);
				}

				ImGui::DragFloat3("LightDir", glm::value_ptr(lightDir));
				ImGui::DragFloat3("Plane rot", glm::value_ptr(rot));

			}
			ImGui::End();

			Renderer::BeginScene(&clearInfo);
			Renderer::SetShadowLightDirection(lightDir);
			Renderer::SubmitDirectionalLight(lightDir, { 1, 1, 1, 1 });
			Renderer::SubmitMesh({ 0, -1, 0 }, { 0, 0, 0 }, { 50, 1, 50, }, &cube, 0);
			Renderer::SubmitMesh({ 0, 1.9f, -3 }, glm::radians(rot), { 1, 1, 1, }, &plane, planeMat);
			Renderer::SubmitMesh({ 3, 0, 0 }, { 0, 0, 0 }, { 1, 1, 1, }, &knight2, stoneMat);
			Renderer::EndScene();
		}
		context->SwapBuffers();
	}
}