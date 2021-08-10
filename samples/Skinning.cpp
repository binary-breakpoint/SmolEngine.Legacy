#include "Skinning.h"

#include <FrostiumCore.h>

#ifdef FROSTIUM_SMOLENGINE_IMPL
using namespace SmolEngine;
#else
using namespace Frostium;
#endif

GraphicsContext*    context = nullptr;

uint32_t            stoneMaterialID = 0;
uint32_t            metalMaterialID = 0;

Mesh                jetMesh;
Mesh                characterMesh;

AnimationController* jet_animController = nullptr;
AnimationController* character_animController = nullptr;

void LoadMeshes()
{
	Mesh::Create("Assets/plane.gltf", &jetMesh);
	Mesh::Create("Assets/CesiumMan.gltf", &characterMesh);
}

void LoadAnimations()
{
	jet_animController = new AnimationController();
	character_animController = new AnimationController();

	/* generates animation.ozz and skeleton.ozz files from gltf model*/
	//OzzImporter::ImportGltf("Assets/CesiumMan.gltf");

	AnimationClipCreateInfo animInfo;
	animInfo.AnimationPath = "Assets/CesiumMan_animation.ozz";
	animInfo.SkeletonPath = "Assets/CesiumMan_skeleton.ozz";
	animInfo.ModelPath = "Assets/CesiumMan.gltf";
	animInfo.ClipInfo.Speed = 1.2f;

	character_animController->AddClip(animInfo, "run");

	animInfo.AnimationPath = "Assets/JetAnim.ozz";
	animInfo.SkeletonPath = "Assets/Jet_Skeleton.ozz";
	animInfo.ModelPath = "Assets/plane.gltf";

	jet_animController->AddClip(animInfo, "fly");
}

void LoadMaterials()
{
	JobsSystemInstance::BeginSubmition();
	{
		JobsSystemInstance::Schedule([]()
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
			stoneMaterialID = MaterialLibrary::GetSinglenton()->Add(&materialCI, "stone");
		});

		JobsSystemInstance::Schedule([]()
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

			metalMaterialID = MaterialLibrary::GetSinglenton()->Add(&materialCI, "metal");
		});
	}
	JobsSystemInstance::EndSubmition();
	DeferredRenderer::UpdateMaterials();
}

void GenerateSkyBox()
{
	DynamicSkyProperties sky;
	DeferredRenderer::SetDynamicSkyboxProperties(sky);
}

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

	DirectionalLight dirLight = {};
	dirLight.IsActive = true;
	dirLight.Color = { 0.6, 0.2, 0.4, 1 };

	context = new GraphicsContext(&info);
	context->SetEventCallback([&](Event& e){ if (e.IsType(EventType::WINDOW_CLOSE)) process = false;});
	context->SetDebugLogCallback([&](const std::string&& msg, LogLevel level) { std::cout << msg << "\n"; });

	LoadMaterials();
	LoadMeshes();
	LoadAnimations();
	GenerateSkyBox();

	while (process)
	{
		context->ProcessEvents();
		float deltaTime = context->CalculateDeltaTime();

		if (context->IsWindowMinimized())
			continue;

		context->UpdateSceneInfo();
		context->BeginFrame(deltaTime);
		{
			ImGui::Begin("Skinning Sample");
			{
				ImGui::DragFloat4("LightDir", glm::value_ptr(dirLight.Direction));

				ImGui::Checkbox("Play", &character_animController->GetActiveClip()->GetProperties().bPlay);
				ImGui::Checkbox("Loop", &character_animController->GetActiveClip()->GetProperties().bLoop);
				ImGui::InputFloat("Speed", &character_animController->GetActiveClip()->GetProperties().Speed);

				float t = character_animController->GetActiveClip()->GetTimeRatio();
				if (ImGui::DragFloat("Time", &t, 0.1f, 0.0f, character_animController->GetActiveClip()->GetDuration()))
					character_animController->GetActiveClip()->SetTimeRatio(t);
			}
			ImGui::End();

			DeferredRenderer::BeginScene(&clearInfo);
			DeferredRenderer::SubmitDirLight(&dirLight);
			DeferredRenderer::SubmitMesh({ 0, 3.9f, -3 }, glm::radians(glm::vec3(-90.0f, 0.0f, 0.0f)), { 1, 1, 1, }, &jetMesh, metalMaterialID, true, jet_animController);
			DeferredRenderer::SubmitMesh({ 3, 2, 0 }, { 0, 0, 0 }, { 5, 5, 5, }, &characterMesh, stoneMaterialID, true, character_animController);
			DeferredRenderer::EndScene();

		}
		context->SwapBuffers();
	}
}