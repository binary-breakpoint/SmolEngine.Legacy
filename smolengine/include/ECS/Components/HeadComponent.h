#pragma once
#include "ECS/Actor.h"
#include "ECS/Components/BaseComponent.h"

#include <string>
#include <vector>

namespace cereal
{
	class access;
}

namespace SmolEngine
{
	struct HeadComponent: public BaseComponent
	{
		HeadComponent() = default;
		HeadComponent(uint32_t id)
			: BaseComponent(id) {}
		HeadComponent(const std::string& name, const std::string tag, uint32_t id)
			:Name(name), Tag(tag), ActorID(id) {}


		Ref<Actor>              Parent = nullptr;
		bool                    bEnabled = true;
		uint32_t                ParentID = 0;
		uint32_t                ActorID = 0;
		uint32_t                ComponentsCount = 0;
		std::string             Name = "";
		std::string             Tag = "";
		std::vector<Ref<Actor>> Childs;

	private:
		friend class cereal::access;
		friend class EditorLayer;
		friend class WorldAdmin;
		friend class Prefab;

		template<typename Archive>
		void serialize(Archive& archive)
		{
			archive(ActorID, ParentID, Name, Tag, bEnabled, Parent, Childs, ComponentID, ComponentsCount);
		}
	};
}