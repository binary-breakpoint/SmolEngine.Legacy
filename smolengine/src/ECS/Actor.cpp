#include "stdafx.h"
#include "ECS/Actor.h"
#include "ECS/WorldAdmin.h"
#include "ECS/Components/HeadComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Scene.h"

namespace SmolEngine
{
	Actor::Actor(entt::entity id)
		: m_Entity(id)
	{
	 
	}

	const std::string& Actor::GetName() const
	{
		return GetHead()->Name;
	}

	const std::string& Actor::GetTag() const
	{
		return GetHead()->Tag;
	}

	uint32_t Actor::GetID() const
	{
		return GetHead()->ActorID;
	}

	uint32_t Actor::GetChildsCount() const
	{
		const HeadComponent* info = GetHead();
		return static_cast<uint32_t>(info->Childs.size());
	}

	Ref<Actor> Actor::GetChildByName(const std::string& name)
	{
		HeadComponent* info = GetInfo();
		for (auto& actor : info->Childs)
		{
			if (actor->GetName() == name)
				return actor;
		}

		return nullptr;
	}

	Ref<Actor> Actor::GetChildByIndex(uint32_t index)
	{
		HeadComponent* info = GetInfo();
		if (index < info->Childs.size())
			return info->Childs[index];

		return nullptr;
	}

	std::vector<Ref<Actor>>& Actor::GetChilds()
	{
		HeadComponent* info = GetInfo();
		return info->Childs;
	}

	uint32_t Actor::GetComponentsCount() const
	{
		return GetHead()->ComponentsCount;
	}

	bool Actor::SetChild(Ref<Actor>& child)
	{
		HeadComponent* p_info = GetInfo();
		bool found = std::find(p_info->Childs.begin(), p_info->Childs.end(), child) != p_info->Childs.end();
		if (found)
			return false;

		HeadComponent* c_info = child->GetInfo();
		// Removes old parent
		if (c_info->Parent != nullptr)
		{
			TransformComponent* transform = c_info->Parent->GetComponent<TransformComponent>();
			HeadComponent* old_parent_info = c_info->Parent->GetInfo();
			transform->TSender.disconnect<&Actor::OnTransformUpdate>(child.get());

			auto& parent_childs = old_parent_info->Childs;
			if (std::find(parent_childs.begin(), parent_childs.end(), child) != parent_childs.end())
			{
				parent_childs.erase(std::remove(parent_childs.begin(), parent_childs.end(), child));

			}
			c_info->Parent = nullptr;
		}

		// Calculates DeltaPos
		TransformComponent* transform = GetComponent<TransformComponent>();
		TransformComponent* childT = child->GetComponent<TransformComponent>();
		childT->DeltaPos = transform->WorldPos - childT->WorldPos;

		// Connect OnUpdate signal
		transform->TSender.connect<&Actor::OnTransformUpdate>(child.get());

		// Adds new parent
		uint32_t myID = GetID();
		Ref<Actor> myHandle = WorldAdmin::GetSingleton()->GetActiveScene()->FindActorByID(myID);
		c_info->Parent = myHandle;
		c_info->ParentID = myID;
		// Adds new child
		p_info->Childs.emplace_back(child);
		return true;
	}

	bool Actor::SetName(const std::string& name)
	{
		uint32_t myID = GetID();
		Ref<Actor> myActor = WorldAdmin::GetSingleton()->GetActiveScene()->FindActorByID(myID);
		return WorldAdmin::GetSingleton()->ChangeActorName(myActor, name);
	}

	bool Actor::RemoveChildAtIndex(uint32_t index)
	{
		HeadComponent* info = GetInfo();

		if (index < info->Childs.size())
		{
			auto& child = info->Childs[index];
			this->GetComponent<TransformComponent>()->TSender.disconnect<&Actor::OnTransformUpdate>(child.get());
			child->GetComponent<TransformComponent>()->DeltaPos = glm::vec3(0.0f);

			info->Childs.erase(info->Childs.begin() + index);
			return true;
		}

		return false;
	}

	void Actor::OnTransformUpdate(Actor* parent)
	{
		TransformComponent* childT = GetComponent<TransformComponent>();
		TransformComponent* parentT = parent->GetComponent<TransformComponent>();

		childT->WorldPos = parentT->WorldPos - childT->DeltaPos;
		if(!childT->TSender.empty())
			childT->OnUpdate.publish(this);
	}

	void Actor::OnDestroy()
	{
		HeadComponent* info = GetInfo();

		if (info->Parent != nullptr)
		{
			TransformComponent* transform = info->Parent->GetComponent<TransformComponent>();
			HeadComponent* parent_info = info->Parent->GetInfo();
			transform->TSender.disconnect<&Actor::OnTransformUpdate>(this);

			auto& parent_childs = parent_info->Childs;
			parent_childs.erase(std::remove_if(parent_childs.begin(), parent_childs.end(), [this](const Ref<Actor>& another) {return another->GetID() == GetID(); }));
			info->Parent = nullptr;
		}

		for (auto& child : GetChilds())
		{
			HeadComponent* info = child->GetInfo();
			info->Parent = nullptr;
			info->ParentID = 0;
		}
	}

	HeadComponent* Actor::GetInfo()
	{
		return GetComponent<HeadComponent>();
	}

	Ref<Actor> Actor::GetParent() const
	{
		return GetHead()->Parent;
	}

	const glm::vec3& Actor::GetPosition()
	{
		return GetComponent<TransformComponent>()->WorldPos;
	}

	const glm::vec3& Actor::GetRotation()
	{
		return GetComponent<TransformComponent>()->Rotation;
	}

	const glm::vec3& Actor::GetScale()
	{
		return GetComponent<TransformComponent>()->Scale;
	}

	Ref<Actor> Actor::FindActorByID(uint32_t id)
	{
		return WorldAdmin::GetSingleton()->GetActiveScene()->FindActorByID(id);
	}

	void Actor::SetPosition(const glm::vec3& pos)
	{
		TransformComponent* transform = GetComponent<TransformComponent>();
		HeadComponent* head = GetComponent<HeadComponent>();

		transform->WorldPos = pos;
		if (head->Parent != nullptr)
		{
			TransformComponent* p_transform = head->Parent->GetComponent<TransformComponent>();
			transform->DeltaPos = p_transform->WorldPos - transform->WorldPos;
		}

		if (!transform->TSender.empty())
			transform->OnUpdate.publish(this);
	}

	void Actor::SetRotation(const glm::vec3& rot)
	{
		GetComponent<TransformComponent>()->Rotation = rot;
	}

	void Actor::SetScale(const glm::vec3& scale)
	{
		GetComponent<TransformComponent>()->Scale = scale;
	}

	const HeadComponent* Actor::GetHead() const
	{
		return WorldAdmin::GetSingleton()->GetActiveScene()->GetComponent<HeadComponent>(m_Entity);
	}
}