#include "stdafx.h"
#include "ECS/Scene.h"
#include "ECS/Actor.h"
#include "ECS/Prefab.h"
#include "ECS/Systems/ScriptingSystem.h"
#include "ECS/Systems/PhysicsSystem.h"
#include "ECS/Components/Include/Components.h"
#include "ECS/Components/Singletons/GraphicsEngineSComponent.h"
#include "ECS/Components/Singletons/WorldAdminStateSComponent.h"

#include "Multithreading/JobsSystem.h"
#include "Pools/PrefabPool.h"

#include <cereal/archives/json.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/memory.hpp>

namespace SmolEngine
{
	Scene::Scene(const Scene& another)
	{

	}

	void Scene::Create(const std::string& filePath)
	{
		m_Registry = entt::registry();
		m_Entity = m_Registry.create();

		m_State = &m_Registry.emplace<SceneStateComponent>(m_Entity);
		m_State->FilePath = filePath;

		m_Registry.on_construct<DirectionalLightComponent>().connect<&Scene::OnConstruct_DirectionalLightComponent>(this);
		m_Registry.on_construct<PostProcessingComponent>().connect<&Scene::OnConstruct_PostProcessingComponent>(this);
		m_Registry.on_construct<SkyLightComponent>().connect<&Scene::OnConstruct_SkyLightComponent>(this);
		m_Registry.on_construct<ScriptComponent>().connect<&Scene::OnConstruct_ScriptComponent>(this);
		m_Registry.on_construct<MeshComponent>().connect<&Scene::OnConstruct_MeshComponent>(this);
		m_Registry.on_construct<Texture2DComponent>().connect<&Scene::OnConstruct_Texture2DComponent>(this);
		m_Registry.on_construct<AudioSourceComponent>().connect<&Scene::OnConstruct_AudioSourceComponent>(this);
		m_Registry.on_construct<TransformComponent>().connect<&Scene::OnConstruct_TransformComponent>(this);
	}

	void Scene::Free()
	{
		m_State = nullptr;
		m_Registry.clear();
	}

	Ref<Actor> Scene::CreateActor(const std::string& name_, const std::string& tag_)
	{
		std::string name = name_ == "" ? "Actor#" + std::to_string(m_State->LastActorID) : name_;
		std::string tag = tag_ == "" ? "Default" : tag;

		const auto& it = m_State->ActorNameSet.find(name);
		if (it != m_State->ActorNameSet.end())
		{
			DebugLog::LogError("[Scene]: Actor {} already exist!", name);
			return nullptr;
		}

		Ref<Actor> actor = std::make_shared<Actor>();
		actor->m_Entity = m_Registry.create();
		uint32_t id = m_State->LastActorID;

		HeadComponent& head = m_Registry.emplace<HeadComponent>(actor->m_Entity);
		head.ComponentID = 0;
		head.ComponentsCount++;
		head.ActorID = id;
		head.Name = name;
		head.Tag = tag;

		AddComponent<TransformComponent>(actor);
		m_State->ActorNameSet[name] = actor;
		m_State->ActorIDSet[id] = actor;
		m_State->LastActorID++;
		m_State->Actors.emplace_back(actor);
		return actor;
	}

	Ref<Actor> Scene::FindActorByName(const std::string& name)
	{
		const auto& it = m_State->ActorNameSet.find(name);
		if (it != m_State->ActorNameSet.end())
			return it->second;

		return nullptr;
	}

	Ref<Actor> Scene::FindActorByTag(const std::string& tag)
	{
		const auto& it = std::find_if(m_State->Actors.begin(), m_State->Actors.end(), 
			[&](const Ref<Actor>& another) {return tag == another->GetTag(); });

		if (it != m_State->Actors.end()) { return *it; }
		else { return nullptr; }
	}

	Ref<Actor> Scene::FindActorByID(uint32_t id)
	{
		auto& it = m_State->ActorIDSet.find(id);
		if (it != m_State->ActorIDSet.end())
			return it->second;

		return nullptr;
	}

	void Scene::GetActors(std::vector<Ref<Actor>>& outList)
	{
		uint32_t count = static_cast<uint32_t>(m_State->Actors.size());

		outList.resize(count);
		for(uint32_t i = 0; i < count; ++i)
			outList[i] = m_State->Actors[i];
	}

	void Scene::GetActorsByTag(const std::string& tag, std::vector<Ref<Actor>>& outList)
	{
		uint32_t count = static_cast<uint32_t>(m_State->Actors.size());
		for (uint32_t i = 0; i < count; ++i)
		{
			auto actor = m_State->Actors[i];
			if (tag == actor->GetTag())
				outList.push_back(actor);
		}
	}

	void Scene::DuplicateActor(Ref<Actor>& actor)
	{
		auto newObj = CreateActor(actor->GetName() + "_D", actor->GetTag());
		auto newT = newObj->GetComponent<TransformComponent>();
		auto oldT = actor->GetComponent<TransformComponent>();

		*newT = *oldT;
	}

	bool Scene::DeleteActor(Ref<Actor>& actor)
	{
		if (actor != nullptr)
		{
			bool found = std::find(m_State->ResetList.begin(), m_State->ResetList.end(), actor) != m_State->ResetList.end();
			if (!found)
			{
				m_State->ResetList.push_back(actor);
				return true;
			}
		}

		return false;
	}

	Ref<Prefab> Scene::LoadPrefab(const std::string& filePath)
	{
		return PrefabPool::ConstructFromPath(filePath);
	}

	Ref<Actor> Scene::InstantiatePrefab(const Ref<Prefab>& ref, const glm::vec3& pos, const glm::vec3& rot, const glm::vec3& scale)
	{
		return ref->Instantiate(this, pos, rot, scale);
	}

	void Scene::OnTick()
	{
		// Destroy entities
		if (m_State->ResetList.size() > 0)
		{
			for (auto& actor : m_State->ResetList)
			{
				actor->OnDestroy();
				ScriptingSystem::OnDestroy(actor.get());
				PhysicsSystem::OnDestroy(actor);

				m_State->ActorNameSet.erase(actor->GetName());
				m_State->ActorIDSet.erase(actor->GetID());

				m_Registry.destroy(*actor);
				m_State->Actors.erase(std::remove_if(m_State->Actors.begin(), m_State->Actors.end(),
					[&](Ref<Actor>& elem) { return elem == actor; }), m_State->Actors.end());

				actor.reset();
			}

			m_State->ResetList.clear();
		}
	}

	bool Scene::Save(const std::string& filePath)
	{
		if (SaveEX(filePath, m_Registry))
		{
			m_State->FilePath = filePath;
			return true;
		}

		return false;
	}

	void Scene::OnConstruct_Complete(entt::registry& registry)
	{
		// Connect signals

		// TransformComponent signals
		{
			auto& view = registry.view<TransformComponent, HeadComponent>();
			for (auto& entity : view)
			{
				TransformComponent* transform = &view.get<TransformComponent>(entity);
				HeadComponent* head = &view.get<HeadComponent>(entity);
				for (auto& child : head->Childs)
				{
					transform->TSender.connect<&Actor::OnTransformUpdate>(child);
				}
			}
		}
	}

	template<typename T>
	void CopyComponent(entt::registry& registry, entt::entity id, entt::registry& another, entt::entity another_id)
	{
		T* component = another.try_get<T>(another_id);
		if (component)
		{
			registry.emplace<T>(id, *component);
		}
	}

	bool Scene::UpdateRegistry(entt::registry& copy_registry, entt::registry& registry)
	{
		JobsSystem::BeginSubmition();
		copy_registry.each([&](entt::entity another_entity)
		{
			auto new_id = registry.create();

			CopyComponent<HeadComponent>(registry, new_id, copy_registry, another_entity);
			CopyComponent<CameraComponent>(registry, new_id, copy_registry, another_entity);
			CopyComponent<ScriptComponent>(registry, new_id, copy_registry, another_entity);
			CopyComponent<SkyLightComponent>(registry, new_id, copy_registry, another_entity);
			CopyComponent<DirectionalLightComponent>(registry, new_id, copy_registry, another_entity);
			CopyComponent<Texture2DComponent>(registry, new_id, copy_registry, another_entity);
			CopyComponent<AudioSourceComponent>(registry, new_id, copy_registry, another_entity);
			CopyComponent<TransformComponent>(registry, new_id, copy_registry, another_entity);
			CopyComponent<CanvasComponent>(registry, new_id, copy_registry, another_entity);
			CopyComponent<Rigidbody2DComponent>(registry, new_id, copy_registry, another_entity);
			CopyComponent<MeshComponent>(registry, new_id, copy_registry, another_entity);
			CopyComponent<PointLightComponent>(registry, new_id, copy_registry, another_entity);
			CopyComponent<SpotLightComponent>(registry, new_id, copy_registry, another_entity);
			CopyComponent<PostProcessingComponent>(registry, new_id, copy_registry, another_entity);
			CopyComponent<RigidbodyComponent>(registry, new_id, copy_registry, another_entity);
		});
		JobsSystem::EndSubmition();
		PBRFactory::UpdateMaterials();
		return true;
	}

	bool Scene::CopySceneEX(entt::registry& registry, entt::registry& another)
	{
		std::stringstream copyStorage;
		{
			cereal::JSONOutputArchive output{ copyStorage };
			entt::snapshot{ another }.entities(output).component<
				HeadComponent, CameraComponent,
				ScriptComponent, SkyLightComponent, DirectionalLightComponent,
				Texture2DComponent, AudioSourceComponent, TransformComponent,
				CanvasComponent, Rigidbody2DComponent, MeshComponent,
				PointLightComponent, SpotLightComponent, SceneStateComponent, PostProcessingComponent, RigidbodyComponent>(output);
		}

		registry.clear();

		JobsSystem::BeginSubmition();
		{
			cereal::JSONInputArchive input{ copyStorage };

			entt::snapshot_loader{ registry }.entities(input).component<
				HeadComponent, CameraComponent,
				ScriptComponent, SkyLightComponent, DirectionalLightComponent,
				Texture2DComponent, AudioSourceComponent, TransformComponent,
				CanvasComponent, Rigidbody2DComponent, MeshComponent,
				PointLightComponent, SpotLightComponent, SceneStateComponent, PostProcessingComponent, RigidbodyComponent>(input).orphans();
		}
		JobsSystem::EndSubmition();
		return true;
	}

	bool Scene::SaveEX(const std::string& filePath, entt::registry& registry)
	{
		std::stringstream storageRegistry;
		{
			cereal::JSONOutputArchive output{ storageRegistry };
			entt::snapshot{ registry }.entities(output).component<
				HeadComponent, CameraComponent,
				ScriptComponent, SkyLightComponent, DirectionalLightComponent,
				Texture2DComponent, AudioSourceComponent, TransformComponent,
				CanvasComponent, Rigidbody2DComponent, MeshComponent,
				PointLightComponent, SpotLightComponent, SceneStateComponent, PostProcessingComponent, RigidbodyComponent>(output);
		}

		std::ofstream myfile(filePath);
		if (myfile.is_open())
		{
			myfile << storageRegistry.str();
			myfile.close();
			return true;
		}

		DebugLog::LogError("[Scene]: Could not write to a file: {}", filePath);
		return false;
	}

	bool Scene::LoadEX(const std::string& filePath, entt::registry& registry, bool connect_signals)
	{
		std::ifstream file(filePath);
		std::stringstream buffer;
		if (!file)
		{
			DebugLog::LogError("[Scene]: Could not open the file: {}", filePath);
			return false;
		}

		buffer << file.rdbuf();
		file.close();

		/* The registry must be cleared before writing new data */
		registry.clear();

		JobsSystem::BeginSubmition();
		{
			cereal::JSONInputArchive regisrtyInput{ buffer };

			entt::snapshot_loader{ registry }.entities(regisrtyInput).component<
				HeadComponent, CameraComponent,
				ScriptComponent, SkyLightComponent, DirectionalLightComponent,
				Texture2DComponent, AudioSourceComponent, TransformComponent,
				CanvasComponent, Rigidbody2DComponent, MeshComponent,
				PointLightComponent, SpotLightComponent, SceneStateComponent, PostProcessingComponent, RigidbodyComponent>(regisrtyInput).orphans();
		}
		JobsSystem::EndSubmition();
		PBRFactory::UpdateMaterials();

		if (connect_signals)
		{
			OnConstruct_Complete(registry);
		}

		return true;
	}

	void Scene::OnConstruct_PostProcessingComponent(entt::registry& registry, entt::entity entity)
	{
		PostProcessingComponent* component = &registry.get<PostProcessingComponent>(entity);
		RendererStateEX& state = RendererStorage::GetState();

		state.Bloom = component->Bloom;
		state.FXAA = component->FXAA;
	}

	void Scene::OnConstruct_SkyLightComponent(entt::registry& registry, entt::entity entity)
	{
		SkyLightComponent* component = &registry.get<SkyLightComponent>(entity);
		GraphicsEngineSComponent* frostium = GraphicsEngineSComponent::Get();
		RendererStateEX& state = RendererStorage::GetState();

		if (!component->CubeMapPath.empty())
		{
			TextureCreateInfo texCI = {};
			if (texCI.Load(component->CubeMapPath))
			{
				component->CubeMap = Texture::Create();
				component->CubeMap->LoadAsCubeFromKtx(&texCI);

				RendererStorage::SetStaticSkybox(component->CubeMap);
				return;
			}
		}

		RendererStorage::SetDynamicSkybox(component->SkyProperties, frostium->ViewProj.Projection, true);
		state.IBL = component->IBLProperties;
	}

	void Scene::OnConstruct_DirectionalLightComponent(entt::registry& registry, entt::entity entity)
	{
		DirectionalLightComponent* component = &registry.get<DirectionalLightComponent>(entity);
		RendererDrawList::SubmitDirLight(dynamic_cast<DirectionalLight*>(component));
	}

	void Scene::OnConstruct_ScriptComponent(entt::registry& registry, entt::entity entity)
	{
		ScriptComponent* component = &registry.get<ScriptComponent>(entity);
		ScriptingSystem::OnConstruct(component);
	}

	void Scene::OnConstruct_MeshComponent(entt::registry& registry, entt::entity entity)
	{
		MeshComponent* component = &registry.get<MeshComponent>(entity);

		// Mesh loading
		JobsSystem::Schedule([component]()
		{

		});
	}

	void Scene::OnConstruct_Texture2DComponent(entt::registry& registry, entt::entity entity)
	{
		Texture2DComponent* component = &registry.get<Texture2DComponent>(entity);
		JobsSystem::Schedule([component]()
		{
			component->Load(component->TexturePath);
		});
	}

	void Scene::OnConstruct_AudioSourceComponent(entt::registry& registry, entt::entity entity)
	{
		AudioSourceComponent* component = &registry.get<AudioSourceComponent>(entity);
		component->Initialize();

		JobsSystem::Schedule([component]()
		{
			for (auto& info : component->Clips)
			{
				component->LoadClip(&info.m_CreateInfo);
			}
		});
	}

	void Scene::OnConstruct_TransformComponent(entt::registry& registry, entt::entity entity)
	{
		TransformComponent* component = &registry.get<TransformComponent>(entity);

		component->OnUpdate = {};
		component->TSender = { component->OnUpdate };
	}

	bool Scene::Load(const std::string& filePath)
	{
		if (LoadEX(filePath, m_Registry))
		{
			m_State = GetStateComponent();
			m_State->FilePath = filePath;
			return true;
		}

		return false;
	}

	SceneStateComponent* Scene::GetStateComponent()
	{
		return m_Registry.try_get<SceneStateComponent>(m_Entity);
	}

	SceneStateComponent* Scene::GetSceneState()
	{
		return m_State;
	}

	entt::registry& Scene::GetRegistry()
	{
		return m_Registry;
	}

	bool Scene::AddCppScript(Ref<Actor>& actor, const std::string& script_name)
	{
		return ScriptingSystem::AttachNativeScript(actor, script_name);
	}

	bool Scene::AddCSharpScript(Ref<Actor>& actor, const std::string& class_name)
	{
		return ScriptingSystem::AttachCSharpScript(actor, class_name);
	}

}