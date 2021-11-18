#include "stdafx.h"
#include "ECS/WorldAdmin.h"

#include "ECS/Components/Include/Components.h"
#include "ECS/Systems/RendererSystem.h"
#include "ECS/Systems/Physics2DSystem.h"
#include "ECS/Systems/PhysicsSystem.h"
#include "ECS/Systems/AudioSystem.h"
#include "ECS/Systems/UISystem.h"
#include "ECS/Systems/ScriptingSystem.h"

#include "ECS/Components/Singletons/AudioEngineSComponent.h"
#include "ECS/Components/Singletons/Box2DWorldSComponent.h"
#include "ECS/Components/Singletons/ProjectConfigSComponent.h"
#include "ECS/Components/Singletons/ScriptingSystemStateSComponent.h"
#include "ECS/Components/Singletons/WorldAdminStateSComponent.h"
#include "ECS/Components/Singletons/Bullet3WorldSComponent.h"
#include "ECS/Components/Singletons/GraphicsEngineSComponent.h"

#include "Multithreading/JobsSystem.h"
#include "Asset/AssetManager.h"
#include "Scripting/CSharp/MonoContext.h"

namespace SmolEngine
{
	WorldAdmin::WorldAdmin()
	{
		s_World = this;
		LoadStaticComponents();

		MonoContext::GetSingleton()->SetOnReloadCallback(std::bind_front(&WorldAdmin::OnHotReload, this));
		m_State->m_InPlayMode = false;
	}

	void WorldAdmin::OnBeginWorld()
	{
		m_State->m_InPlayMode = true;

		Physics2DSystem::OnBeginWorld();
		PhysicsSystem::OnBeginWorld();
		AudioSystem::OnBeginWorld();
		ScriptingSystem::OnBeginWorld();
	}

	void WorldAdmin::OnBeginFrame()
	{
		// Extracting Camera
		entt::registry& registry = GetActiveScene()->GetRegistry();
		const auto& cameraGroup = registry.view<CameraComponent, TransformComponent>();
		for (const auto& entity : cameraGroup)
		{
			const auto& [camera, transform] = cameraGroup.get<CameraComponent, TransformComponent>(entity);
			if (camera.bPrimaryCamera == false || m_State->m_LevelEditorActive) { continue; }

			camera.CalculateView(&transform);
			GraphicsEngineSComponent* engine = GraphicsEngineSComponent::Get();

			engine->ViewProj.View = camera.ViewMatrix;
			engine->ViewProj.Projection = camera.ProjectionMatrix;
			engine->ViewProj.NearClip = camera.zNear;
			engine->ViewProj.FarClip = camera.zFar;
			engine->ViewProj.CamPos = glm::vec4(transform.WorldPos, 1.0f);
			engine->ViewProj.SkyBoxMatrix = glm::mat4(glm::mat3(camera.ViewMatrix));

			AudioSystem::OnUpdate(transform.WorldPos);

			// At the moment we support only one viewport
			break;
		}
	}

	void WorldAdmin::OnEndFrame()
	{
		Scene* scene = GetActiveScene();
		scene->OnTick();

		RendererSystem::OnRender();
	}

	void WorldAdmin::OnEndWorld()
	{
		m_State->m_InPlayMode = false;

		ScriptingSystem::OnEndWorld();
		Physics2DSystem::OnEndWorld();
		PhysicsSystem::OnEndWorld();
		AudioSystem::OnEndWorld();
	}

	void WorldAdmin::OnUpdate(float deltaTime)
	{
		if (m_State->m_InPlayMode)
		{
			ScriptingSystem::OnUpdate(deltaTime);
			Physics2DSystem::OnUpdate(deltaTime);
			PhysicsSystem::OnUpdate(deltaTime);

			Physics2DSystem::UpdateTransforms();
			PhysicsSystem::UpdateTransforms();
		}
	}

	void WorldAdmin::OnEvent(Event& e)
	{
		if (e.IsType(EventType::WINDOW_RESIZE))
		{
			entt::registry& registry = GetActiveScene()->GetRegistry();
			const auto& cameraGroup = registry.view<CameraComponent>();
			for (const auto& entity : cameraGroup)
			{
				const auto& camera = cameraGroup.get<CameraComponent>(entity);

				WindowResizeEvent* resize = e.Cast<WindowResizeEvent>();
				CameraComponent::OnResize(resize->GetWidth(), resize->GetHeight());
			}
		}
	}

	void WorldAdmin::OnHotReload()
	{
		//temp
	}

	void WorldAdmin::OnWorldReset()
	{
		RendererDrawList::SetDefaultState();
		RendererStorage::SetDefaultState();

		AssetManager::Clear();
		PBRFactory::ClearMaterials();
		PBRFactory::AddDefaultMaterial();
		PBRFactory::UpdateMaterials();

		DirectionalLight DirLight;
		RendererDrawList::SubmitDirLight(&DirLight);
	}

	bool WorldAdmin::LoadScene(const std::string& filePath, bool is_reload)
	{
		std::string path = filePath;
		std::ifstream file(path);
		if (!file)
		{
			DebugLog::LogError("[WorldAdmin]: Could not open the file: {}", path);
			return false;
		}

		if (is_reload == false) { OnWorldReset(); }

		Scene* activeScene = GetActiveScene();
		if (activeScene->Load(path) == true)
		{
			m_State->m_CurrentRegistry = &activeScene->GetRegistry();
			DebugLog::LogWarn(is_reload ? "[WorldAdmin]: Scene reloaded successfully" : "[WorldAdmin]: Scene loaded successfully");
			return true;
		}

		return false;
	}

	bool WorldAdmin::SwapScene(uint32_t index)
	{
		return false; //temp
	}

	bool WorldAdmin::SaveCurrentScene()
	{
		SceneStateComponent* sceneState = GetActiveScene()->GetSceneState();
		if(std::filesystem::exists(sceneState->FilePath))
			return SaveScene(sceneState->FilePath);

		return false;
	}

	bool WorldAdmin::LoadLastSceneState()
	{
		SceneStateComponent* sceneState = GetActiveScene()->GetSceneState();
		return LoadScene(sceneState->FilePath, true);
	}

	bool WorldAdmin::LoadSceneRuntime(const std::string& path)
	{
		if(m_State->m_CurrentRegistry != nullptr)
			OnEndWorld();

		Scene* scene = &m_State->m_Scenes[0];
		scene->Create("empty");
		m_State->m_CurrentRegistry = &scene->GetRegistry();
		m_State->m_ActiveScene = scene;

		if (LoadScene(path))
		{
			OnBeginWorld();
			return true;
		}

		return false;
	}

	bool WorldAdmin::SaveScene(const std::string& filePath)
	{
		return GetActiveScene()->Save(filePath);
	}

	bool WorldAdmin::IsInPlayMode()
	{
		return m_State->m_InPlayMode;
	}

	bool WorldAdmin::CreateScene(const std::string& filePath)
	{
		OnWorldReset();

		if (GetActiveScene()->m_Registry.alive() > 0)
		{
			DynamicSkyProperties props;
			props.SunPosition = glm::vec4(1, -11, 0, 0);
			props.NumCirrusCloudsIterations = 0;

			GraphicsEngineSComponent* engine = GraphicsEngineSComponent::Get();
			RendererStorage::SetDynamicSkybox(props, engine->ViewProj.View, true);
		}

		Scene* scene = &m_State->m_Scenes[0];
		scene->Create(filePath);

		m_State->m_CurrentRegistry = &scene->GetRegistry();
		m_State->m_ActiveScene = scene;
		return true;
	}

	Scene* WorldAdmin::GetActiveScene()
	{
		return m_State->m_ActiveScene;
	}

	bool WorldAdmin::LoadStaticComponents()
	{
		m_GlobalEntity = m_GlobalRegistry.create();

		m_GlobalRegistry.emplace<AudioEngineSComponent>(m_GlobalEntity);
		m_GlobalRegistry.emplace<Box2DWorldSComponent>(m_GlobalEntity);
		m_GlobalRegistry.emplace<Bullet3WorldSComponent>(m_GlobalEntity);
		m_GlobalRegistry.emplace<ScriptingSystemStateSComponent>(m_GlobalEntity);
		m_GlobalRegistry.emplace<GraphicsEngineSComponent>(m_GlobalEntity);
		m_GlobalRegistry.emplace<ProjectConfigSComponent>(m_GlobalEntity);
		m_GlobalRegistry.emplace<WorldAdminStateSComponent>(m_GlobalEntity);

		AudioSystem::m_State = &m_GlobalRegistry.get<AudioEngineSComponent>(m_GlobalEntity);
		Physics2DSystem::m_State = &m_GlobalRegistry.get<Box2DWorldSComponent>(m_GlobalEntity);
		PhysicsSystem::m_State = &m_GlobalRegistry.get<Bullet3WorldSComponent>(m_GlobalEntity);
		ScriptingSystem::m_State = &m_GlobalRegistry.get<ScriptingSystemStateSComponent>(m_GlobalEntity);
		RendererSystem::m_State = &m_GlobalRegistry.get<GraphicsEngineSComponent>(m_GlobalEntity);
		m_State = &m_GlobalRegistry.get<WorldAdminStateSComponent>(m_GlobalEntity);

		AudioSystem::m_World = m_State;
		PhysicsSystem::m_World = m_State;
		ScriptingSystem::m_World = m_State;
		RendererSystem::m_World = m_State;
		Physics2DSystem::m_World = m_State;

		m_State->m_ActiveScene = &m_State->m_Scenes[0];
		return true;
	}

	bool WorldAdmin::ChangeActorName(Ref<Actor>& actor, const std::string& name)
	{
		SceneStateComponent* state = GetActiveScene()->GetSceneState();
		std::string oldName = actor->GetName();
		auto& it = state->ActorNameSet.find(name);
		if (it == state->ActorNameSet.end())
		{
			state->ActorNameSet.erase(oldName);
			state->ActorNameSet[name] = actor;
			actor->GetInfo()->Name = name;
			return true;
		}

		return false;
	}
}