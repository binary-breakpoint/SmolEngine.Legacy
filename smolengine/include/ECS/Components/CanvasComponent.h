#pragma once
#include "ECS/Components/BaseComponent.h"

namespace cereal
{
	class access;
}

namespace SmolEngine
{
	struct CanvasComponent: public BaseComponent
	{
		CanvasComponent() = default;
		CanvasComponent(uint32_t id)
			:BaseComponent(id) {}

	private:

		friend class cereal::access;
		friend class EditorLayer;
		friend class WorldAdmin;

		template<typename Archive>
		void serialize(Archive& archive)
		{
			archive(ComponentID);
		}
	};
}