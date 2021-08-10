#include "Skinning.h"

#include <FrostiumCore.h>

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

	AnimationController animController;
	AnimationController animController2;

	AnimationClipCreateInfo animInfo;
	animInfo.AnimationPath = "Assets/CesiumMan_animation.ozz";
	animInfo.SkeletonPath = "Assets/CesiumMan_skeleton.ozz";
	animInfo.ModelPath = "Assets/CesiumMan.gltf";

	animController.AddClip(animInfo, "run");

	animInfo.AnimationPath = "Assets/JetAnim.ozz";
	animInfo.SkeletonPath = "Assets/Jet_Skeleton.ozz";
	animInfo.ModelPath = "Assets/plane.gltf";
	animController2.AddClip(animInfo, "fly");

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

	bool debug = false;
	float zNear = 1.0f;
	float zFar = 350.0f;
	float lightFOV = 45.0f;
	float animSpeed = 1.0f;
	float lightIntensity = 1.0f;

	RendererStateEX& state = DeferredRenderer::GetState();

	DirectionalLight dirLight = {};
	dirLight.IsActive = true;
	dirLight.Color = { 0.6, 0.2, 0.4, 1 };
	//dirLight.IsCastShadows = false;

	PointLight pointLight = {};
	pointLight.Intensity = 20;
	pointLight.Raduis = 100;
	pointLight.Color = { 0.2f, 0.2f, 0.7f, 1.0 };

	// Load models
	Mesh plane = {};
	Mesh dummy = {};
	Mesh::Create("Assets/plane.gltf", &plane);
	Mesh::Create("Assets/CesiumMan.gltf", &dummy);

	TextureCreateInfo textureCI = {};
	textureCI.FilePath = "Assets/DirtMaskTextureExample.png";

	Texture dirtMask = {};
	Texture::Create(&textureCI, &dirtMask);
	DeferredRenderer::SetDirtMask(&dirtMask, 1.0f, 0.1f);

	// Load Materials
	uint32_t stoneMat = 0;
	uint32_t planeMat = 0;
	{
		JobsSystemInstance::BeginSubmition();
		{
			JobsSystemInstance::Schedule([&stoneMat]()
			{
				MaterialCreateInfo materialCI{};
				TextureCreateInfo textureCI{};

				textureCI.FilePath = "Assets/materials/stone/Tiles087_1K_Color.png";
				materialCI.SetTexture(MaterialTexture::Albedro, &textureCI);

				textureCI.FilePath = "Assets/materials/stone/Tiles087_1K_Normal.png";
				materialCI.SetTexture(MaterialTexture::Normal, &textureCI);

				textureCI.FilePath = "Assets/materials/stone/Tiles087_1K_Roughness.png";
				materialCI.SetTexture(MaterialTexture::Roughness, &textureCI);

				textureCI.FilePath = "Assets/materials/stone/Tiles087_1K_AmbientOcclusion.png";
				materialCI.SetTexture(MaterialTexture::AO, &textureCI);

				materialCI.SetMetalness(0.1f);
				materialCI.SetEmissionStrength(1.1f);
				stoneMat = MaterialLibrary::GetSinglenton()->Add(&materialCI, "stone");
			});

			JobsSystemInstance::Schedule([&planeMat]()
			{
				MaterialCreateInfo materialCI{};
				TextureCreateInfo textureCI{};

				textureCI.FilePath = "Assets/materials/metal_2/Metal012_1K_Color.png";
				materialCI.SetTexture(MaterialTexture::Albedro, &textureCI);

				textureCI.FilePath = "Assets/materials/metal_2/Metal012_1K_Normal.png";
				materialCI.SetTexture(MaterialTexture::Normal, &textureCI);

				textureCI.FilePath = "Assets/materials/metal_2/Metal012_1K_Roughness.png";
				materialCI.SetTexture(MaterialTexture::Roughness, &textureCI);

				textureCI.FilePath = "Assets/materials/metal_2/Metal012_1K_Metalness.png";
				materialCI.SetTexture(MaterialTexture::Metallic, &textureCI);

				materialCI.SetMetalness(0.5f);
				materialCI.SetRoughness(0.9f);

				planeMat = MaterialLibrary::GetSinglenton()->Add(&materialCI, "metal");
			});
		}
		JobsSystemInstance::EndSubmition();
		DeferredRenderer::UpdateMaterials();
	}

	DynamicSkyProperties sky;
	DeferredRenderer::SetDynamicSkyboxProperties(sky);
	glm::vec3 sunPos = glm::vec4(50, 100, 40, 0);

	while (process)
	{
		context->ProcessEvents();
		float deltaTime = context->CalculateDeltaTime();

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
				static bool ssao = true;

				if (ImGui::Combo("##FF", &debug_val, "None\0Albedro\0Position\0Normals\0Materials\0Emission\0ShadowMap\0ShadowMapCood\0AO\0"))
				{
					state.eDebugView = (DebugViewFlags)debug_val;
				}

				ImGui::Checkbox("Grid", &state.bDrawGrid);
				ImGui::Checkbox("Skybox", &state.bDrawSkyBox);
				ImGui::Checkbox("Bloom", &state.bBloom);
				ImGui::Checkbox("IBL", &state.bIBL);
				ImGui::Checkbox("FXAA", &state.bFXAA);
				ImGui::InputFloat("Exposure", &state.Bloom.Exposure);
				ImGui::InputFloat("Threshold", &state.Bloom.Threshold);
				ImGui::InputFloat("Strength", &state.Bloom.Strength);
				ImGui::InputFloat("Scale", &state.Bloom.Scale);
				ImGui::Checkbox("Debug Draw", &debug);
				ImGui::DragFloat4("LightDir", glm::value_ptr(dirLight.Direction));

				ImGui::Checkbox("Anim Play", &animController.GetActiveClip()->GetProperties().bPlay);
				ImGui::Checkbox("Anim Loop", &animController.GetActiveClip()->GetProperties().bLoop);
				ImGui::InputFloat("Anim Speed", &animController.GetActiveClip()->GetProperties().Speed);

				float t = animController.GetActiveClip()->GetTimeRatio();
				if (ImGui::DragFloat("Anim Time", &t, 0.1f, 0.0f, animController.GetActiveClip()->GetDuration()))
					animController.GetActiveClip()->SetTimeRatio(t);
			}
			ImGui::End();

			DeferredRenderer::BeginScene(&clearInfo);
			DeferredRenderer::SubmitDirLight(&dirLight);
			DeferredRenderer::SubmitMesh({ 0, 3.9f, -3 }, glm::radians(rot), { 1, 1, 1, }, &plane, planeMat, true, &animController2);
			DeferredRenderer::SubmitMesh({ 3, 2, 0 }, { 0, 0, 0 }, { 5, 5, 5, }, &dummy, stoneMat, true, &animController);
			DeferredRenderer::EndScene();
			
			if (debug)
			{
				DebugRenderer::BeginDebug();
				DebugRenderer::DrawCapsule(1, 2, 1, { 1, 2, 1 }, { 3,2,0 }, { 1, 1, 1 });
				DebugRenderer::EndDebug();
			}

		}
		context->SwapBuffers();
	}
}