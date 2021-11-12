#pragma once
#include "ECS/Components/BaseComponent.h"
#include "Renderer/RendererShared.h"

namespace cereal
{
	class access;
}

namespace SmolEngine
{
	struct PointLightComponent: public BaseComponent, PointLight
	{
		PointLightComponent() = default;
		PointLightComponent(uint32_t id)
			:BaseComponent(id), PointLight() {}
				      
	private:
		friend class cereal::access;

		template<typename Archive>
		void serialize(Archive& archive)
		{
			archive(Color.r, Color.g, Color.b, Color.a,
				Position.x, Position.y, Position.z, Position.w,
				Raduis,
				Bias,
				Intensity, 
				IsActive,
				ComponentID);
		}
	};
}