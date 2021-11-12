#include "stdafx.h"
#include "ECS/Prefab.h"
#include "ECS/Scene.h"
#include "ECS/ComponentsCore.h"
#include "ECS/Components/Singletons/ScriptingSystemStateSComponent.h"
#include "Scripting/CPP/MetaContext.h"
#include "Scripting/CSharp/MonoContext.h"

namespace SmolEngine
{
	Ref<Actor> FindTopNode(Ref<Actor>& actor)
	{
		auto& parent = actor->GetParent();
		if (parent != nullptr)
		{
			return FindTopNode(parent);
		}

		return actor;
	}

    bool Prefab::CreateFromActor(Ref<Actor>& actor, Scene* scene, const std::string& savePath)
    {
        if (Scene::CopySceneEX(m_Data, scene->GetRegistry()))
        {
            std::vector<uint32_t> out_ids;
            FindEntityID(actor, scene, out_ids);
            m_Data.each([&, this](entt::entity entity)
            {
                uint32_t entt_id = (uint32_t)entity;
                bool found = std::find(out_ids.begin(), out_ids.end(), entt_id) != out_ids.end();
                if (!found)
                {
                    m_Data.destroy(entity);
                }
            });

            return Scene::SaveEX(savePath, m_Data);
        }

        return false;
    }

    bool Prefab::LoadFromFile(const std::string& filePath)
    {
        return Scene::LoadEX(filePath, m_Data, false);
    }

	Ref<Actor> Prefab::Instantiate(Scene* scene, const glm::vec3& pos, const glm::vec3& rot, const glm::vec3& scale)
    {
        if (IsGood())
        {
			entt::registry& registry = scene->GetRegistry();
            if (Scene::UpdateRegistry(m_Data, registry))
            {
				struct ActorResolveInfo
				{
					uint32_t oldID = 0;
					Ref<Actor> actor = nullptr;
				};

				SceneStateComponent& state = *scene->GetSceneState();
				std::vector<ActorResolveInfo> new_actors;

				registry.each([&, this](entt::entity entity)
				{
					if (registry.valid(entity))
					{
						bool found = std::find_if(state.Actors.begin(), state.Actors.end(), [&, this](const Ref<Actor>& another)
							{ return (uint32_t)entity == *(uint32_t*)another.get(); }) != state.Actors.end();

						if (!found)
						{
							Ref<Actor> actor = std::make_shared<Actor>(entity);
							HeadComponent* head = scene->GetComponent<HeadComponent>(entity);
							if (head)
							{
								uint32_t id = state.LastActorID;
								std::string name = head->Name;

								auto& it = state.ActorNameSet.find(head->Name);
								if (it != state.ActorNameSet.end())
								{
									name = name + "_ID#" + std::to_string(id);
								}

								ActorResolveInfo info;
								info.actor = actor;
								info.oldID = head->ActorID;

								head->Name = name;
								head->ActorID = id;
								head->Childs.clear();
								head->Parent = nullptr;

								state.ActorNameSet[name] = actor;
								state.ActorIDSet[id] = actor;
								state.LastActorID++;

								new_actors.emplace_back(info);
								state.Actors.emplace_back(actor);
							}
						}
					}
				});

				Ref<Actor> parent = nullptr;

				// Reassign child nodes
				for (auto& info : new_actors)
				{
					HeadComponent* head = info.actor->GetComponent<HeadComponent>();
					uint32_t targetID = head->ParentID;

					if (!parent)
						parent = info.actor;

					if (targetID > 0)
					{
						auto& it = std::find_if(new_actors.begin(), new_actors.end(), [&](const ActorResolveInfo& another) { return targetID == another.oldID; });
						if (it != new_actors.end())
						{

							it->actor->SetChild(info.actor);
						}
					}
				}

				parent = FindTopNode(parent);
				parent->SetPosition(pos);

				if(scale != glm::vec3(1))
					parent->SetScale(scale);

				if(rot != glm::vec3(0))
					parent->SetRotation(rot);

				for (auto& info : new_actors)
				{
					auto rb = info.actor->GetComponent<RigidbodyComponent>();
					if (rb)
					{
						ComponentHandler::ValidateRigidBodyComponent(rb, info.actor);
					}

					auto script = info.actor->GetComponent<ScriptComponent>();
					if (script)
					{
						auto state = ScriptingSystemStateSComponent::GetSingleton();

						if (script->CppScripts.size() > 0)
							state->m_MetaContext->OnBegin(script);

						if (script->CSharpScripts.size() > 0)
							state->m_MonoContext->OnBegin(script);
					}
				}

                return parent;
            }
        }

        return nullptr;
    }

	bool Prefab::IsGood() const
	{
		return m_Data.alive() > 0;
	}

	void Prefab::Free()
	{
		m_Data.clear();
	}

    void Prefab::FindEntityID(Ref<Actor>& actor, Scene* scene, std::vector<uint32_t>& out_ids)
    {
        uint32_t id = *(uint32_t*)actor.get();
        if (std::find(out_ids.begin(), out_ids.end(), id) == out_ids.end())
        {
            out_ids.push_back(id);
        }

        auto& childs = actor->GetChilds();
        for (auto& child : childs)
            FindEntityID(child, scene, out_ids);
    }
}