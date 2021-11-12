#pragma once
#include "ECS/Components/BaseComponent.h"
#include "Renderer/RendererShared.h"

namespace cereal
{
	class access;
}

namespace SmolEngine
{
	enum class ShadowType : int
	{
		None,
		Hard,
		Soft
	};

	struct DirectionalLightComponent: public BaseComponent, public DirectionalLight
	{
		DirectionalLightComponent() = default;
		DirectionalLightComponent(uint32_t id)
			:BaseComponent(id) {}

		ShadowType eShadowType = ShadowType::None;

	private:
		int ShadowsFlags = 0;

		friend class cereal::access;
		friend class EditorLayer;

		template<typename Archive>
		void serialize(Archive& archive)
		{
			archive(eShadowType,
				Color.r, Color.g, Color.b, Color.a,
				Direction.x, Direction.y, Direction.z, Direction.w,
				Intensity,
				Bias,
				zFar,
				zNear,
				lightFOV,
				IsActive,
				IsCastShadows,
				IsUseSoftShadows);
		}
	};
}