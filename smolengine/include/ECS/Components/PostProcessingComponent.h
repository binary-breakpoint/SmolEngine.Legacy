#pragma once
#include "ECS/Components/BaseComponent.h"
#include "Renderer/RendererShared.h"

namespace cereal
{
	class access;
}

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
			archive(Bloom.DirtIntensity, Bloom.Enabled, Bloom.Exposure, Bloom.Intensity, Bloom.Knee, Bloom.SkyboxMod, Bloom.Threshold, Bloom.UpsampleScale,
				FXAA.EdgeThresholdMax, FXAA.EdgeThresholdMin, FXAA.Iterations, FXAA.SubPixelQuality, FXAA.Enabled);
		}
	};
}