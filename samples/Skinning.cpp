#include "Skinning.h"

#include <Common/Mesh.h>
#include <Common/Input.h>
#include <GraphicsContext.h>
#include <MaterialLibrary.h>
#include <DebugRenderer.h>
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
		info.eShadowMapSize = ShadowMapSize::SIZE_8;
	}

	bool process = true;
	ClearInfo clearInfo = {};
	context = new GraphicsContext(&info);
	context->SetEventCallback([&](Event& e)
		{
			if (e.IsType(EventType::WINDOW_CLOSE))
				process = false;
		});

	glm::vec3 lightDir = glm::vec3(0.0f);
	glm::vec3 rot = glm::vec3(-90.0f, 0.0f, 0.0f);

	bool playAnim = false;
	bool reset = false;
	bool wireframes = false;
	float zNear = 1.0f;
	float zFar = 350.0f;
	float lightFOV = 45.0f;
	float animSpeed = 1.0f;
	float lightIntensity = 1.0f;

	RenderingState state = {};
	DirectionalLight dirLight = {};
	dirLight.IsActive = true;
	dirLight.IsCastShadows = true;

	// Load models
	Mesh plane = {};
	Mesh dummy = {};
	Mesh::Create("Assets/plane.gltf", &plane);
	Mesh::Create("Assets/CesiumMan.gltf", &dummy);
	Mesh* cube = GraphicsContext::GetSingleton()->GetBoxMesh();
	Mesh* sphere = GraphicsContext::GetSingleton()->GetSphereMesh();

	// Load Materials
	uint32_t stoneMat;
	uint32_t planeMat;
	{
		MaterialCreateInfo materialCI = {};

		{
			materialCI.SetTexture(MaterialTexture::Albedro, "Assets/materials/stone/Tiles087_1K_Color.png");
			materialCI.SetTexture(MaterialTexture::Normal, "Assets/materials/stone/Tiles087_1K_Normal.png");
			materialCI.SetTexture(MaterialTexture::Roughness, "Assets/materials/stone/Tiles087_1K_Roughness.png");
			materialCI.SetTexture(MaterialTexture::AO, "Assets/materials/stone/Tiles087_1K_AmbientOcclusion.png");
			materialCI.SetMetalness(0.1f);

			stoneMat = MaterialLibrary::GetSinglenton()->Add(&materialCI);
		}

		{
			materialCI = {};
			materialCI.SetTexture(MaterialTexture::Albedro, "Assets/materials/plane/Metal021_2K_Color.png");
			materialCI.SetTexture(MaterialTexture::Normal, "Assets/materials/plane/Metal021_2K_Normal.png");
			materialCI.SetTexture(MaterialTexture::Roughness, "Assets/materials/plane/Metal021_2K_Roughness.png");
			materialCI.SetTexture(MaterialTexture::Metallic, "Assets/materials/plane/Metal021_2K_Metalness.png");

			planeMat = MaterialLibrary::GetSinglenton()->Add(&materialCI);
		}
	}

	Renderer::UpdateMaterials();
	plane.SetMaterialID(planeMat, true);

	AnimationProperties* defaultProp = plane.GetAnimationProperties(0);
	AnimationProperties* defaultProp2 = dummy.GetAnimationProperties(0);
	defaultProp2->SetActive(true);

	while (process)
	{
		context->ProcessEvents();
		DeltaTime deltaTime = context->CalculateDeltaTime();

		if (context->IsWindowMinimized())
			continue;
		bool selected = true;
		context->UpdateSceneInfo();
		context->BeginFrame(deltaTime);
		{
			ImGui::Begin("Skinning Sample");
			{
				if (ImGui::Checkbox("Grid", &state.bDrawGrid))
					Renderer::SetRenderingState(&state);

				if (ImGui::Checkbox("Skybox", &state.bDrawSkyBox))
					Renderer::SetRenderingState(&state);

				if (ImGui::Checkbox("Bloom Pass", &state.bBloomPass))
					Renderer::SetRenderingState(&state);

				ImGui::Checkbox("Wireframes", &wireframes);
				if (ImGui::InputFloat("LightIntensity", &lightIntensity))
				{
					dirLight.Intensity = lightIntensity;
					Renderer::SubmitDirLight(&dirLight);
				}

				if (ImGui::InputFloat("Znear", &zNear))
				{
					dirLight.zNear = zNear;
					Renderer::SubmitDirLight(&dirLight);
				}

				if (ImGui::InputFloat("ZFar", &zFar))
				{
					dirLight.zFar = zFar;
					Renderer::SubmitDirLight(&dirLight);
				}

				if (ImGui::InputFloat("LightFOV", &lightFOV))
				{
					dirLight.lightFOV = lightFOV;
					Renderer::SubmitDirLight(&dirLight);
				}

				if (ImGui::DragFloat3("LightDir", glm::value_ptr(lightDir)))
				{
					dirLight.Direction = glm::vec4(lightDir, 1);
					Renderer::SubmitDirLight(&dirLight);
				}

				if (ImGui::Checkbox("Play", &playAnim))
				{
					defaultProp->SetActive(true);
					reset = false;
				}

				ImGui::SameLine();
				if (ImGui::Checkbox("Reset", &reset))
				{
					plane.ResetAnimation(0);
					playAnim = false;
				}

				if (!playAnim && defaultProp->IsActive())
				{
					defaultProp->SetActive(false);
				}

				if (ImGui::InputFloat("Anim Speed", &animSpeed))
				{
					defaultProp2->SetSpeed(animSpeed);
				}

				ImGui::DragFloat3("Plane rot", glm::value_ptr(rot));
			}
			ImGui::End();

			Renderer::BeginScene(&clearInfo);
			Renderer::SubmitMesh({ -5, 5, 0 }, { 0, 0, 0 }, { 2, 2, 2, }, sphere, planeMat);
			Renderer::SubmitMesh({ 0, 1, 0 }, { 0, 0, 0 }, { 50, 1, 50, }, cube, 0);
			Renderer::SubmitMesh({ 0, 3.9f, -3 }, glm::radians(rot), { 1, 1, 1, }, &plane, planeMat);
			Renderer::SubmitMesh({ 3, 2, 0 }, { 0, 0, 0 }, { 5, 5, 5, }, &dummy, stoneMat);
			Renderer::EndScene();

			if (wireframes)
			{
				DebugRenderer::BeginDebug();
				DebugRenderer::DrawWireframes({ 0, 1, 0 }, { 0, 0, 0 }, { 50, 1, 50, }, cube);
				DebugRenderer::EndDebug();
			}
		}
		context->SwapBuffers();
	}
}