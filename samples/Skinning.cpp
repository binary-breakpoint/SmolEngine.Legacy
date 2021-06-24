#include "Skinning.h"

#include <Common/Mesh.h>
#include <Common/Input.h>
#include <GraphicsContext.h>
#include <MaterialLibrary.h>
#include <DebugRenderer.h>
#include <DeferredRenderer.h>

#include <Utils/Utils.h>

#ifdef FROSTIUM_SMOLENGINE_IMPL
using namespace SmolEngine;
#else
using namespace Frostium;
#endif

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
		info.Flags = Features_Renderer_3D_Flags | Features_ImGui_Flags | Features_Renderer_Debug_Flags;
		info.ResourcesFolderPath = "../resources/";
		info.pWindowCI = &windoInfo;
		info.pDefaultCamera = camera;
		info.eMSAASamples = MSAASamples::SAMPLE_COUNT_4;
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
	bool debug = false;
	float zNear = 1.0f;
	float zFar = 350.0f;
	float lightFOV = 45.0f;
	float animSpeed = 1.0f;
	float lightIntensity = 1.0f;

	RendererState state = {};
	state.Lighting.UseIBL = false;

	DirectionalLight dirLight = {};
	dirLight.IsActive = false;
	dirLight.IsCastShadows = false;
	DeferredRenderer::SubmitDirLight(&dirLight);

	PointLight pointLight = {};
	pointLight.Intensity = 20;
	pointLight.Raduis = 100;
	pointLight.Color = { 0.2f, 0.2f, 0.7f, 1.0 };

	// Load models
	Mesh plane = {};
	Mesh dummy = {};
	Mesh::Create("Assets/plane.gltf", &plane);
	Mesh::Create("Assets/CesiumMan.gltf", &dummy);
	Mesh* cube = GraphicsContext::GetSingleton()->GetBoxMesh();
	Mesh* sphere = GraphicsContext::GetSingleton()->GetSphereMesh();

	Texture dirtMask = {};
	Texture::Create("Assets/DirtMaskTextureExample.png", &dirtMask);
	DeferredRenderer::SetDirtMask(&dirtMask, 1.0f);

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
			materialCI.SetEmissionStrength(5.0f);
			stoneMat = MaterialLibrary::GetSinglenton()->Add(&materialCI, "stone");
		}

		{
			materialCI = {};
			materialCI.SetTexture(MaterialTexture::Albedro, "Assets/materials/plane/Metal021_2K_Color.png");
			materialCI.SetTexture(MaterialTexture::Normal, "Assets/materials/plane/Metal021_2K_Normal.png");
			materialCI.SetTexture(MaterialTexture::Roughness, "Assets/materials/plane/Metal021_2K_Roughness.png");
			materialCI.SetTexture(MaterialTexture::Metallic, "Assets/materials/plane/Metal021_2K_Metalness.png");

			planeMat = MaterialLibrary::GetSinglenton()->Add(&materialCI, "metal");
		}
	}

	DeferredRenderer::UpdateMaterials();
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
				static int debug_val = 0;
				static int type  = 0;
				static bool ibl = true;

				if (ImGui::Combo("##FF", &debug_val, "None\0Albedro\0Position\0Normals\0Materials\0Emission\0ShadowMap\0ShadowMapCood\0AO\0"))
				{
					state.eDebugView = (DebugViewFlags)debug_val;
					DeferredRenderer::SetRendererState(&state);
				}

				if (ImGui::Checkbox("Grid", &state.bDrawGrid))
					DeferredRenderer::SetRendererState(&state);

				if (ImGui::Checkbox("Skybox", &state.bDrawSkyBox))
					DeferredRenderer::SetRendererState(&state);

				if (ImGui::Checkbox("Bloom", &state.bBloom))
					DeferredRenderer::SetRendererState(&state);


				if (ImGui::Checkbox("IBL", &ibl))
				{
					state.Lighting.UseIBL = ibl;
					DeferredRenderer::SetRendererState(&state);
				}

				if (ImGui::Checkbox("Vertical Bloom", &state.bVerticalBloom))
					DeferredRenderer::SetRendererState(&state);

				if (ImGui::Checkbox("FXAA", &state.bFXAA))
					DeferredRenderer::SetRendererState(&state);

				if (ImGui::InputFloat("Exposure", &state.Bloom.Exposure))
					DeferredRenderer::SetRendererState(&state);

				if (ImGui::InputFloat("Threshold", &state.Bloom.Threshold))
					DeferredRenderer::SetRendererState(&state);

				if (ImGui::InputFloat("Strength", &state.Bloom.Strength))
					DeferredRenderer::SetRendererState(&state);

				if (ImGui::InputFloat("Scale", &state.Bloom.Scale))
					DeferredRenderer::SetRendererState(&state);

				ImGui::Checkbox("Debug Draw", &debug);

				if (ImGui::DragFloat3("LightDir", glm::value_ptr(lightDir)))
				{
					pointLight.Position = glm::vec4(lightDir, 1);
					DeferredRenderer::SubmitPointLight(&pointLight);
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

			DeferredRenderer::BeginScene(&clearInfo);
			DeferredRenderer::SubmitMesh({ -5, 5, 0 }, { 0, 0, 0 }, { 2, 2, 2, }, sphere, planeMat);
			DeferredRenderer::SubmitPointLight(&pointLight);
			DeferredRenderer::SubmitMesh({ -10, 1, 0 }, { 0, 0, 0 }, { 3, 3, 3 }, cube);
			//DeferredRenderer::SubmitMesh({ 0, 3.9f, -3 }, glm::radians(rot), { 1, 1, 1, }, &plane, planeMat);
			DeferredRenderer::SubmitMesh({ 3, 2, 0 }, { 0, 0, 0 }, { 5, 5, 5, }, &dummy, stoneMat);
			DeferredRenderer::EndScene();
			
			if (debug)
			{
				DebugRenderer::BeginDebug();
				DebugRenderer::DrawWireframes({ 0, 1, 0 }, { 0, 0, 0 }, { 50, 1, 50, }, cube);
				DebugRenderer::DrawCapsule(1, 2, 1, { 1, 2, 1 }, { 3,2,0 }, { 1, 1, 1 });
				DebugRenderer::EndDebug();
			}

		}
		context->SwapBuffers();
	}
}