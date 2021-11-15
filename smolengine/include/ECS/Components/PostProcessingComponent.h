#pragma once
#include "ECS/Components/BaseComponent.h"
#include "Renderer/RendererShared.h"

namespace SmolEngine
{
	struct PostProcessingComponent: public BaseComponent
	{
		PostProcessingComponent() = default;
		PostProcessingComponent(uint32_t id)
			:BaseComponent(id) {}

		FXAAProperties  FXAA{};
		BloomProperties Bloom{};

	private:
		friend class cereal::access;

		template<typename Archive>
		void serialize(Archive& archive)
		{
			archive(Bloom, FXAA);
		}
	};
}