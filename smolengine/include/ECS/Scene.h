#pragma once
#include "Core/Core.h"

#include <entt/entt.hpp>

namespace SmolEngine
{
	class Actor;
	class Prefab;
	struct SceneStateComponent;

	class Scene
	{
	public:
		Scene() = default;
		~Scene() = default;
		Scene(const Scene& another);

		bool                    Save(const std::string& filePath);
		bool                    Load(const std::string& filePath);
		void                    DuplicateActor(Ref<Actor>& actor);
		bool                    DeleteActor(Ref<Actor>& actor);
		Ref<Prefab>             LoadPrefab(const std::string& filePath);
		Ref<Actor>              InstantiatePrefab(const Ref<Prefab>& ref, const glm::vec3& pos, const glm::vec3& rot = glm::vec3(0.0f), const glm::vec3& scale = glm::vec3(1.0f));
		Ref<Actor>              CreateActor(const std::string& name = "", const std::string& tag = "");
		Ref<Actor>              FindActorByName(const std::string& name);
		Ref<Actor>              FindActorByTag(const std::string& tag);
		Ref<Actor>              FindActorByID(uint32_t id);
		void                    GetActors(std::vector<Ref<Actor>>& outList);
		void                    GetActorsByTag(const std::string& tag, std::vector<Ref<Actor>>& outList);
		SceneStateComponent*    GetSceneState();
		entt::registry&         GetRegistry();

		template<typename T, typename... Args>
		T* AddComponent(Ref<Actor>& actor, Args&&... args)
		{
			return AddComponent<T>(actor->m_Entity, args...);
		}

		template<typename T, typename... Args>
		T* AddComponent(entt::entity entt_id, Args&&... args)
		{
			if (HasComponent<T>(entt_id))
			{
				return nullptr;
			}

			HeadComponent* head = GetComponent<HeadComponent>(entt_id);
			auto& component = m_Registry.emplace<T>(entt_id, std::forward<Args>(args)...);
			component.ComponentID = head->ComponentsCount;
			head->ComponentsCount++;
			return &component;
		}

		template<typename T>
		bool DestroyComponent(Ref<Actor>& actor)
		{
			return DestroyComponent<T>(actor->m_Entity);
		}

		template<typename T>
		bool DestroyComponent(entt::entity entt_id)
		{
			if (HasComponent<T>(entt_id))
			{
				m_Registry.remove<T>(entt_id);
				return true;
			}

			return false;
		}

		template<typename T>
		bool HasComponent(Ref<Actor>& actor)
		{
			return HasComponent<T>(actor->m_Entity);
		}

		template<typename T>
		bool HasComponent(entt::entity entt_id)
		{
			return m_Registry.try_get<T>(entt_id) != nullptr;
		}

		template<typename T>
		T* GetComponent(Ref<Actor>& actor)
		{
			return GetComponent<T>(actor->m_Entity);
		}

		template<typename T>
		T* GetComponent(entt::entity entt_id)
		{
			return m_Registry.try_get<T>(entt_id);
		}

		bool AddCppScript(Ref<Actor>& actor, const std::string& script_name);
		bool AddCSharpScript(Ref<Actor>& actor, const std::string& class_name);

	private:
		static bool             UpdateRegistry(entt::registry& registry, entt::registry& another);
		static bool             CopySceneEX(entt::registry& registry, entt::registry& another);
		static bool             SaveEX(const std::string& filePath, entt::registry& registry);
		static bool             LoadEX(const std::string& filePath, entt::registry& registry, bool connect_signals = true);

		void                    Free();
		void                    Create(const std::string& filePath);
		void                    OnTick();
		static void             OnConstruct_Complete(entt::registry& registry);
		void                    OnConstruct_PostProcessingComponent(entt::registry& registry, entt::entity entity);
		void                    OnConstruct_SkyLightComponent(entt::registry& registry, entt::entity entity);
		void                    OnConstruct_DirectionalLightComponent(entt::registry& registry, entt::entity entity);
		void                    OnConstruct_ScriptComponent(entt::registry& registry, entt::entity entity);
		void                    OnConstruct_MeshComponent(entt::registry& registry, entt::entity entity);
		void                    OnConstruct_Texture2DComponent(entt::registry& registry, entt::entity entity);
		void                    OnConstruct_AudioSourceComponent(entt::registry& registry, entt::entity entity);
		void                    OnConstruct_TransformComponent(entt::registry& registry, entt::entity entity);
		SceneStateComponent*    GetStateComponent();

	private:
		entt::registry          m_Registry{};
		entt::entity            m_Entity{};
		SceneStateComponent*    m_State = nullptr;

	private:				    
		friend class WorldAdmin;
		friend class EditorLayer;
		friend class Prefab;
	};
}