#include "Skinning.h"

#include <FrostiumCore.h>

#ifdef FROSTIUM_SMOLENGINE_IMPL
using namespace SmolEngine;
#else
using namespace Frostium;
#endif

GraphicsContext*    context = nullptr;
RendererStorage*    storage = nullptr;
EditorCamera*       camera = nullptr;

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

	character_animController->AddClip(animInfo, "run", true);

	animInfo.AnimationPath = "Assets/JetAnim.ozz";
	animInfo.SkeletonPath = "Assets/Jet_Skeleton.ozz";
	animInfo.ModelPath = "Assets/plane.gltf";

	jet_animController->AddClip(animInfo, "fly", true);
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

			storage->GetMaterialLibrary().Add(&materialCI, "stone");
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

			storage->GetMaterialLibrary().Add(&materialCI, "metal");
		});
	}
	JobsSystemInstance::EndSubmition();
	storage->OnUpdateMaterials();
}

void GenerateSkyBox()
{
	DynamicSkyProperties sky;
	storage->SetDynamicSkybox(sky, camera->GetProjection(), true);
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

	{
		EditorCameraCreateInfo cameraCI = {};
		camera = new EditorCamera(&cameraCI);
	}

	GraphicsContextInitInfo info = {};
	{
		info.ResourcesFolderPath = "../resources/";
		info.pWindowCI = &windoInfo;
		info.pDefaultCamera = camera;
		info.eMSAASamples = MSAASamples::SAMPLE_COUNT_4;
		info.bVsync = true;
	}

	bool process = true;
	ClearInfo clearInfo = {};
	SceneViewProjection sceneViewProj = {};
	sceneViewProj.Update(camera);

	DirectionalLight dirLight = {};
	dirLight.IsActive = true;
	dirLight.Color = { 0.6, 0.2, 0.4, 1 };

	storage = new RendererStorage();
	context = new GraphicsContext(&info);
	context->SetEventCallback([&](Event& e){ if (e.IsType(EventType::WINDOW_CLOSE)) process = false;});
	context->SetDebugLogCallback([&](const std::string&& msg, LogLevel level) { std::cout << msg << "\n"; });

	storage->Initilize();
	context->PushStorage(storage);

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

		sceneViewProj.Update(camera);
		storage->BeginSubmit(&sceneViewProj);
		{
			storage->SubmitDirLight(&dirLight);
			storage->SubmitMesh({ 0, 3.9f, -3 }, glm::radians(glm::vec3(-90.0f, 0.0f, 0.0f)), { 1, 1, 1, }, &jetMesh, metalMaterialID, true, jet_animController);
			storage->SubmitMesh({ 3, 2, 0 }, { 0, 0, 0 }, { 5, 5, 5, }, &characterMesh, stoneMaterialID, true, character_animController);
		}
		storage->EndSubmit();

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

			RendererDeferred::DrawFrame(&clearInfo, storage);
		}
		context->SwapBuffers();
	}
}