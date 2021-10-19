#include "Skinning.h"

#include <GraphicsCore.h>

using namespace SmolEngine;

GraphicsContext*    context = nullptr;
RendererStorage*    storage = nullptr;
RendererDrawList*   drawList = nullptr;
EditorCamera*       camera = nullptr;

uint32_t            stoneMaterialID = 0;
uint32_t            metalMaterialID = 0;

Mesh                jetMesh;
Mesh                characterMesh;
Mesh*               cubeMesh = nullptr;

AnimationController* jet_animController = nullptr;
AnimationController* character_animController = nullptr;

void LoadMeshes()
{
	cubeMesh = context->GetDefaultMeshes()->Cube.get();
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

			textureCI.FilePath = "Assets/materials/bricks/Bricks061_2K_Color.png";
			materialCI.SetTexture(MaterialTexture::Albedo, &textureCI);

			textureCI.FilePath = "Assets/materials/bricks/Bricks061_2K_Normal.png";
			materialCI.SetTexture(MaterialTexture::Normal, &textureCI);

			textureCI.FilePath = "Assets/materials/bricks/Bricks061_2K_Roughness.png";
			materialCI.SetTexture(MaterialTexture::Roughness, &textureCI);

			textureCI.FilePath = "Assets/materials/bricks/Bricks061_2K_AmbientOcclusion.png";
			materialCI.SetTexture(MaterialTexture::AO, &textureCI);

			materialCI.SetMetalness(1.0f);
			materialCI.SetEmissionStrength(4.0f);

			stoneMaterialID = storage->GetMaterialLibrary().Add(&materialCI, "stone");
		});

		JobsSystemInstance::Schedule([]()
		{
			MaterialCreateInfo materialCI{};
			TextureCreateInfo textureCI{};

			textureCI.FilePath = "Assets/materials/metal_2/Metal012_1K_Color.png";
			materialCI.SetTexture(MaterialTexture::Albedo, &textureCI);

			textureCI.FilePath = "Assets/materials/metal_2/Metal012_1K_Normal.png";
			materialCI.SetTexture(MaterialTexture::Normal, &textureCI);

			textureCI.FilePath = "Assets/materials/metal_2/Metal012_1K_Roughness.png";
			materialCI.SetTexture(MaterialTexture::Roughness, &textureCI);

			textureCI.FilePath = "Assets/materials/metal_2/Metal012_1K_Metalness.png";
			materialCI.SetTexture(MaterialTexture::Metallic, &textureCI);

			materialCI.SetMetalness(0.5f);
			materialCI.SetRoughness(0.9f);

			metalMaterialID = storage->GetMaterialLibrary().Add(&materialCI, "metal");
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
		info.eMSAASamples = MSAASamples::SAMPLE_COUNT_4;
		info.bVsync = true;
	}

	bool process = true;
	ClearInfo clearInfo = {};
	SceneViewProjection viewProj = {};

	DirectionalLight dirLight = {};
	dirLight.IsActive = true;
	dirLight.IsCastShadows = false;

	DebugLog* log = new DebugLog([&](const std::string&& msg, LogLevel level) { std::cout << msg << "\n"; });
	storage = new RendererStorage();
	drawList = new RendererDrawList();
	context = new GraphicsContext(&info);
	context->SetEventCallback([&](Event& e) { if (e.IsType(EventType::WINDOW_CLOSE)) process = false; camera->OnEvent(e); });

	storage->Initilize();
	storage->GetState().bDrawGrid = false;
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

		context->BeginFrame(deltaTime);
		{
			ImGui::Begin("Skinning Sample");
			{
				ImGui::Checkbox("Light", &dirLight.IsActive);
				ImGui::DragFloat4("Light Dir", glm::value_ptr(dirLight.Direction));
				ImGui::ColorEdit4("Light Color", glm::value_ptr(dirLight.Color));
				ImGui::InputFloat("Light Intensity", &dirLight.Intensity);

				ImGui::Checkbox("Bloom", &storage->GetState().Bloom.Enabled);
				ImGui::InputFloat("Threshold", &storage->GetState().Bloom.Threshold);
				ImGui::InputFloat("Knee", &storage->GetState().Bloom.Knee);
				ImGui::InputFloat("SkyboxMo", &storage->GetState().Bloom.SkyboxMod);

				ImGui::Checkbox("Fxaa", &storage->GetState().FXAA.Enabled);
				ImGui::Checkbox("IBL", &storage->GetState().IBL.Enabled);

				ImGui::Checkbox("Play", &character_animController->GetActiveClip()->GetProperties().bPlay);
				ImGui::Checkbox("Loop", &character_animController->GetActiveClip()->GetProperties().bLoop);
				ImGui::InputFloat("Speed", &character_animController->GetActiveClip()->GetProperties().Speed);

				float t = character_animController->GetActiveClip()->GetTimeRatio();
				if (ImGui::DragFloat("Time", &t, 0.1f, 0.0f, character_animController->GetActiveClip()->GetDuration()))
					character_animController->GetActiveClip()->SetTimeRatio(t);
			}
			ImGui::End();

			camera->OnUpdate(deltaTime);
			viewProj.Update(camera);

			drawList->BeginSubmit(&viewProj);
			{
				drawList->SubmitDirLight(&dirLight);
				drawList->SubmitMesh({ 0, 3.9f, -3 }, glm::radians(glm::vec3(-90.0f, 0.0f, 0.0f)), { 1, 1, 1, }, &jetMesh, metalMaterialID, true, jet_animController);
				drawList->SubmitMesh({ 3, 2, 0 }, { 0, 0, 0 }, { 5, 5, 5, }, &characterMesh, metalMaterialID, true, character_animController);

				drawList->SubmitMesh({ -5, 0, 0 }, { 0, 0, 0 }, { 2, 2, 2, }, cubeMesh);
			}
			drawList->EndSubmit();

			RendererDeferred::DrawFrame(&clearInfo, storage, drawList);
		}
		context->SwapBuffers();
	}
}