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
			archive(CubeMapPath, bGeneratePBRMaps, SkyProperties, IBLProperties);
		}
	};
}