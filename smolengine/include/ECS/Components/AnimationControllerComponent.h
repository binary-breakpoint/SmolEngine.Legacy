#pragma once
#include "ECS/Components/BaseComponent.h"
#include "Animation/AnimationController.h"

#include <vector>

namespace cereal
{
	class access;
}

namespace SmolEngine
{
	struct AnimationControllerComponent : public BaseComponent
	{
		AnimationControllerComponent() = default;
		AnimationControllerComponent(uint32_t id)
			:BaseComponent(id) {}

	private:
		bool bShowEditorUI = false;
		std::vector<std::string> Paths;
		Ref<AnimationController> Controller = nullptr;

	private:
		friend class cereal::access;
		friend class EditorLayer;
		friend class ComponentHandler;
		friend class Scene;

		template<typename Archive>
		void serialize(Archive& archive)
		{
			archive(ComponentID, Paths);
		}
	};
}