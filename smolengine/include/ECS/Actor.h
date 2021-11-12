#pragma once

#include "ECS/Scene.h"
#include "ECS/WorldAdmin.h"

#include <string>
#include <vector>

namespace cereal
{
	class access;
}

namespace SmolEngine
{
	struct OutValue;
	struct DefaultBaseTuple;
	struct PhysicsBaseTuple;
	struct CameraBaseTuple;
	struct ResourceTuple;
	struct HeadComponent;

	class Actor
	{
	public:
		Actor() = default;
		Actor(entt::entity id);
		operator entt::entity() const { return m_Entity; }

		// Getters
		const std::string&           GetName() const;
		const std::string&           GetTag() const;
		uint32_t                     GetComponentsCount() const;
		const HeadComponent*         GetHead() const;
		uint32_t                     GetID() const;
		uint32_t                     GetChildsCount() const;
		Ref<Actor>                   GetChildByName(const std::string& name);
		Ref<Actor>                   GetChildByIndex(uint32_t index);
		std::vector<Ref<Actor>>&     GetChilds();
		Ref<Actor>                   GetParent() const;
		const glm::vec3&             GetPosition();
		const glm::vec3&             GetRotation();
		const glm::vec3&             GetScale();
		Ref<Actor>                   FindActorByID(uint32_t id);

		// Setters
		void                         SetPosition(const glm::vec3& pos);
		void                         SetRotation(const glm::vec3& rot);
		void                         SetScale(const glm::vec3& scale);
		bool                         SetChild(Ref<Actor>& child);
		bool                         SetName(const std::string& name);
		bool                         RemoveChildAtIndex(uint32_t index);

		template<typename T>
		T* GetComponent()            { return WorldAdmin::GetSingleton()->GetActiveScene()->GetComponent<T>(m_Entity); }
		template<typename T>
		bool HasComponent()          { return WorldAdmin::GetSingleton()->GetActiveScene()->HasComponent<T>(m_Entity); }
		template<typename T>
		bool DestroyComponent()       { return WorldAdmin::GetSingleton()->GetActiveScene()->DestroyComponent<T>(m_Entity); }

		template<typename T, typename... Args>
		T* AddComponent(Args&&... args) { return WorldAdmin::GetSingleton()->GetActiveScene()->AddComponent<T>(m_Entity, args...); }

	private:
		void                         OnTransformUpdate(Actor* parent);
		void                         OnDestroy();
		HeadComponent*               GetInfo();

	private:
		entt::entity                 m_Entity{};

	private:
		friend class cereal::access;
		friend struct ScriptableObject;
		friend class WorldAdmin;
		friend class Scene;
		friend class EditorLayer;
		friend class ScriptingSystem;
		friend class MonoContext;

		template<typename Archive>
		void serialize(Archive& archive)
		{
			archive(m_Entity);
		}

	};
}