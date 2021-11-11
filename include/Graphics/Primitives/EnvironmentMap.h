#pragma once

#include "Graphics/Primitives/Framebuffer.h"
#include "Graphics/Primitives/GraphicsPipeline.h"
#include "Graphics/Primitives/Texture.h"

#include <glm/glm.hpp>

namespace SmolEngine
{
	struct DynamicSkyProperties
	{
		glm::vec4 RayOrigin = glm::vec4(0, 6372e3f, 0, 0);
		glm::vec4 SunPosition = glm::vec4(1.0f, 1.0f, 0, 0);
		glm::vec4 RayleighScatteringCoeff = glm::vec4(5.5e-6, 13.0e-6, 22.4e-6, 0);

		float SunIntensity = 22.0f;
		// Radius of the planet in meters
		float PlanetRadius = 6371e3f;
		// Radius of the atmosphere in meters
		float AtmosphereRadius = 6471e3;
		float MieScatteringCoeff = 21e-6f;

		// Rayleigh scale height
		float RayleighScale = 8e3f;
		// Mie scale height
		float MieScale = 1.2e3f;
		//Mie preferred scattering direction
		float MieScatteringDirection = 0.758f;
		uint32_t  NumCirrusCloudsIterations = 2;

		uint32_t  NumCumulusCloudsIterations = 0;
	private:
		float pad1 = 1.0f;
		float pad2 = 1.0f;
		float pad3 = 1.0f;
	};

	class EnvironmentMap: public PrimitiveBase
	{
	public:
		void                    Initialize();
		void                    GenerateStatic(Ref<Texture>& cubeMap);
		void                    GenerateDynamic(const glm::mat4& cameraProj = glm::mat4(0));
		void                    UpdateDescriptors();
		void                    SetDimension(uint32_t dim);
		bool                    IsDynamic() const;
		Ref<Texture>            GetCubeMap() const;
		DynamicSkyProperties&   GetDynamicSkyProperties();
		void                    Free() override;
		bool                    IsGood() const override;

	private:
		bool                    m_IsDynamic = false;
		Ref<Texture>            m_CubeMap = nullptr;
		Ref<GraphicsPipeline>   m_GraphicsPipeline = nullptr;
		Ref<Framebuffer>        m_Framebuffer = nullptr;
		uint32_t                m_Dimension = 1024;
		DynamicSkyProperties    m_UBO = {};
	};
}