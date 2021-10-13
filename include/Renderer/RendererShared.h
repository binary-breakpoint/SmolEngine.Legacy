#pragma once
#include "Common/Memory.h"

namespace SmolEngine
{
	struct RendererStorageBase
	{
		virtual void  OnResize(uint32_t width, uint32_t height) {};
		void          Build();

		template<typename T>
		T* Cast() { return dynamic_cast<T*>(this); }

	private:          
		virtual void  Initilize() = 0;
		void          AddStorage();
	};

	struct GLSL_BOOLPAD
	{
		bool data[3];
	};

	struct SceneData;
	struct SceneViewProjection;
	struct CommandBufferStorage;
	struct ClearInfo;

	static const uint32_t max_animations = 100;
	static const uint32_t max_anim_joints = 1000;
	static const uint32_t max_materials = 1000;
	static const uint32_t max_lights = 1000;
	static const uint32_t max_objects = 15000;

#pragma region Shader-Side Structures

	struct DirectionalLight
	{
		glm::vec4      Direction = glm::vec4(67, 56, 0, 0);
		glm::vec4      Color = glm::vec4(1.0);
		float          Intensity = 1.0f;
		float          Bias = 1.0f;
		float          zNear = 1.0f;
		float          zFar = 350.0f;

		float          lightFOV = 45.0f;
		bool           IsActive = false;
		GLSL_BOOLPAD   Pad1;
		bool           IsCastShadows = false;
		GLSL_BOOLPAD   Pad2;
		bool           IsUseSoftShadows = true;
		GLSL_BOOLPAD   Pad3;
	};

	struct PointLight
	{
		glm::vec4      Position = glm::vec4(0);
		glm::vec4      Color = glm::vec4(1.0);
		float          Intensity = 1.0f;
		float          Raduis = 10.0f;
		float          Bias = 1.0f;
		bool           IsActive = true;
	private:
		GLSL_BOOLPAD   Pad1;
	};

	struct SpotLight
	{
		glm::vec4      Position = glm::vec4(0, 0, 0, 0);
		glm::vec4      Direction = glm::vec4(0, 0, 40, 0);
		glm::vec4      Color = glm::vec4(1.0);
		float          Intensity = 10.0f;
		float          CutOff = 40.0f;
		float          OuterCutOff = 5.0f;
		float          Raduis = 10.0f;

		float          Bias = 0.005f;
		bool           IsActive = true;
	private:
		GLSL_BOOLPAD   Pad1;
		uint32_t       Pad2[2];
	};

	struct InstanceData
	{
		uint32_t       MaterialID = 0;
		uint32_t       IsAnimated = false;
		uint32_t       AnimOffset = 0;
		uint32_t       EntityID = 0;
		glm::mat4      ModelView = glm::mat4(1.0f);
	};

	struct IBLProperties
	{
		glm::vec4      AmbientColor = glm::vec4(1.0f);
		float          IBLStrength = 1.0f;
		bool           Enabled = true;
	private:
		GLSL_BOOLPAD   Pad1;
	};

	struct BloomProperties
	{
		float          Threshold = 0.7f;
		float          Knee = 0.1f;
		float          UpsampleScale = 1.0f;
		float          Intensity = 1.0f;
		float          DirtIntensity = 1.0f;
		float          Exposure = 1.0f;
		float          SkyboxMod = 1.0f;
		bool           Enabled = false;
	private:
		GLSL_BOOLPAD   Pad1;
	};

	struct FXAAProperties
	{
		float          EdgeThresholdMin = 0.0312f;
		float          EdgeThresholdMax = 0.125f;
		float          Iterations = 29.0f;
		float          SubPixelQuality = 0.75f;
		// don't use! for internal needs
		glm::vec2      InverseScreenSize = glm::vec2(0.0f);
		bool           Enabled = true;
	private:
		GLSL_BOOLPAD   Pad1;
		float          Pad2;
	};

#pragma endregion

	struct DirtMask
	{
		Texture* Mask = nullptr;
		float    Intensity = 1.0f;
		float    BaseIntensity = 0.1f;
	};

	enum class ShadowMapSize : uint16_t
	{
		SIZE_2,
		SIZE_4,
		SIZE_8,
		SIZE_16
	};

	enum class DebugViewFlags : uint32_t
	{
		None = 0,
		Albedro,
		Position,
		Normals,
		Materials,
		Emission,
		ShadowMap,
		ShadowMapCood,
		AO
	};

	struct RendererStateEX
	{
		bool                   bDrawSkyBox = true;
		bool                   bDrawGrid = true;
		DebugViewFlags         eDebugView = DebugViewFlags::None;
		IBLProperties          IBL = {};
		BloomProperties        Bloom = {};
		FXAAProperties         FXAA = {};
	};
}