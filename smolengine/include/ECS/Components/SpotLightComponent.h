#pragma once
#include "ECS/Components/BaseComponent.h"
#include "Renderer/RendererShared.h"

#include <glm/glm.hpp>

namespace cereal
{
	class access;
}

namespace SmolEngine
{
	struct SpotLightComponent : public BaseComponent, SpotLight
	{
		SpotLightComponent() = default;
		SpotLightComponent(uint32_t id)
			:BaseComponent(id), SpotLight() {}

	private:

		friend class cereal::access;

		template<typename Archive>
		void serialize(Archive& archive)
		{
			archive(Color.r, Color.g, Color.b, Color.a,
				Position.x, Position.y, Position.z,
				Direction.x, Direction.y, Direction.z,
				Intensity, CutOff, OuterCutOff, Raduis, Bias, IsActive, ComponentID);
		}
	};
}