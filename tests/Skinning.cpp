#include "Skinning.h"

#include <GraphicsCore.h>

using namespace SmolEngine;

Ref<GraphicsContext>  context = nullptr;
EditorCamera*         camera = nullptr;

std::pair<Ref<Mesh>, Ref<MeshView>>  jetMesh;
std::pair<Ref<Mesh>, Ref<MeshView>>  characterMesh;

Ref<AnimationController> jet_animController = nullptr;
Ref<AnimationController> character_animController = nullptr;

void LoadMeshes()
{
	jetMesh = MeshPool::ConstructFromFile("Assets/plane.gltf");
	characterMesh = MeshPool::ConstructFromFile("Assets/CesiumMan.gltf");
}

void LoadAnimations()
{
	jet_animController = std::make_shared<AnimationController>();
	character_animController = std::make_shared<AnimationController>();

	/* generates animation.ozz and skeleton.ozz files from gltf model*/
	//OzzImporter::ImportGltf("Assets/CesiumMan.gltf");

	AnimationClipCreateInfo animInfo;
	animInfo.AnimationPath = "Assets/CesiumMan_animation.ozz";
	animInfo.SkeletonPath = "Assets/CesiumMan_skeleton.ozz";
	animInfo.ModelPath = "Assets/CesiumMan.gltf";
	animInfo.ClipInfo.Speed = 1.2f;

	character_animController->AddClip(animInfo, "run", true);
	characterMesh.second->SetAnimationController(character_animController);

	animInfo.AnimationPath = "Assets/JetAnim.ozz";
	animInfo.SkeletonPath = "Assets/Jet_Skeleton.ozz";
	animInfo.ModelPath = "Assets/plane.gltf";

	jet_animController->AddClip(animInfo, "fly", true);
	jetMesh.second->SetAnimationController(jet_animController);
}

void GenerateSkyBox()
{
	DynamicSkyProperties sky;
	RendererStorage::SetDynamicSkybox(sky, camera->GetProjection(), true);
}

int main(int argc, char** argv)
{
	WindowCreateInfo windoInfo = {};
	{
		windoInfo.bFullscreen = false;
		windoInfo.bVSync = false;
		windoInfo.Height = 480;
		windoInfo.Width = 720;
		windoInfo.Title = "Skinning";
	}

	EditorCameraCreateInfo cameraCI = {};
	camera = new EditorCamera(&cameraCI);

	GraphicsContextCreateInfo info = {};
	{
		info.eMSAASamples = MSAASamples::SAMPLE_COUNT_1;
		info.ResourcesFolder = "../resources/";
		info.pWindowCI = &windoInfo;
	}

	ClearInfo clearInfo = {};
	context = GraphicsContext::Create(&info);
	context->SetEventCallback([&](Event& e)
		{
			camera->OnEvent(e);
		});

	LoadMeshes();
	LoadAnimations();
	GenerateSkyBox();

	DirectionalLight dirLight = {};
	dirLight.IsActive = true;
	dirLight.IsCastShadows = true;
	dirLight.Direction = glm::vec4(105.0f, 53.0f, 102.0f, 0);
	RendererDrawList::SubmitDirLight(&dirLight);

	while (context->IsOpen())
	{
		context->ProcessEvents();
		float deltaTime = context->CalculateDeltaTime();

		if (context->IsWindowMinimized())
			continue;

		/*
			Calculates physics, update scripts, etc.
		*/

		camera->OnUpdate(deltaTime);

		context->BeginFrame(deltaTime);
		{
			ImGui::Begin("Skinning Sample");
			{
				ImGui::Checkbox("Play", &character_animController->GetActiveClip()->GetProperties().bPlay);
				ImGui::Checkbox("Loop", &character_animController->GetActiveClip()->GetProperties().bLoop);
				ImGui::InputFloat("Speed", &character_animController->GetActiveClip()->GetProperties().Speed);

				float t = character_animController->GetActiveClip()->GetTimeRatio();
				if (ImGui::DragFloat("Time", &t, 0.1f, 0.0f, character_animController->GetActiveClip()->GetDuration()))
					character_animController->GetActiveClip()->SetTimeRatio(t);
			}
			ImGui::End();

			RendererDrawList::BeginSubmit(camera->GetSceneViewProjection());
			{
				RendererDrawList::SubmitDirLight(&dirLight);
				RendererDrawList::SubmitMesh({ 0, 3.9f, -3 }, glm::radians(glm::vec3(-90.0f, 0.0f, 0.0f)), { 1, 1, 1, }, jetMesh.first, jetMesh.second);
				RendererDrawList::SubmitMesh({ 3, 2, 0 }, { 0, 0, 0 }, { 5, 5, 5, }, characterMesh.first, characterMesh.second);
			}
			RendererDrawList::EndSubmit();

			RendererDeferred::DrawFrame(&clearInfo);
		}
		context->SwapBuffers();
	}
}