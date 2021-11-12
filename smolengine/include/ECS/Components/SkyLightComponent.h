#pragma once
#include "ECS/Components/BaseComponent.h"
#include "Renderer/RendererShared.h"

namespace cereal
{
	class access;
}

namespace SmolEngine
{
	struct SkyLightComponent: public BaseComponent
	{
		SkyLightComponent() = default;
		SkyLightComponent(uint32_t id)
			:BaseComponent(id) {}

		bool                 bGeneratePBRMaps = true;
		std::string          CubeMapPath = "";
		Ref<Texture>         CubeMap = nullptr;
		DynamicSkyProperties SkyProperties{};
		IBLProperties        IBLProperties{};

	private:
		/* internal use */
		int EnvironmentFlags = 0;

	private:
		friend class cereal::access;
		friend class EditorLayer;

		template<typename Archive>
		void serialize(Archive& archive)
		{
			archive(CubeMapPath, bGeneratePBRMaps,

				SkyProperties.AtmosphereRadius,
				SkyProperties.MieScale,
				SkyProperties.MieScatteringCoeff,
				SkyProperties.MieScatteringDirection,
				SkyProperties.PlanetRadius,
				SkyProperties.RayleighScale,
				SkyProperties.RayleighScatteringCoeff.x,
				SkyProperties.RayleighScatteringCoeff.y,
				SkyProperties.RayleighScatteringCoeff.z,
				SkyProperties.RayOrigin.x,
				SkyProperties.RayOrigin.y,
				SkyProperties.RayOrigin.z,
				SkyProperties.SunIntensity,
				SkyProperties.SunPosition.x,
				SkyProperties.SunPosition.y,
				SkyProperties.SunPosition.z,
				SkyProperties.NumCirrusCloudsIterations,
				SkyProperties.NumCumulusCloudsIterations,
				
				IBLProperties.AmbientColor.r, IBLProperties.AmbientColor.g, IBLProperties.AmbientColor.b,
				IBLProperties.IBLStrength,
				IBLProperties.Enabled);
		}
	};
}