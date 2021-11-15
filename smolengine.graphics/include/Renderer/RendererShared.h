#pragma once
#include "Memory.h"

#include "Animation/AnimationController.h"
#include "Primitives/GraphicsPipeline.h"
#include "Primitives/ComputePipeline.h"
#include "Primitives/EnvironmentMap.h"
#include "Primitives/Framebuffer.h"
#include "Primitives/Texture.h"
#include "Primitives/Mesh.h"

#include "Camera/Frustum.h"
#include "Camera/Camera.h"

namespace cereal
{
	class access;
}

namespace SmolEngine
{
	struct PBRHandle;

	struct RendererStorageBase
	{
		RendererStorageBase() = default;
		virtual ~RendererStorageBase() = default;

		virtual void  OnResize(uint32_t width, uint32_t height) {};
		void          Build();

		template<typename T>
		T* Cast() { return dynamic_cast<T*>(this); }

	private:          
		virtual void  Initilize() = 0;
	};

	struct GLSL_BOOLPAD
	{
		bool data[3];
	};

	struct ClearInfo
	{
		bool      bClear = true;
		glm::vec4 color = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
	};

	struct SceneData;
	struct SceneViewProjection;
	struct CommandBufferStorage;

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

		friend class cereal::access;

		template<typename Archive>
		void serialize(Archive& archive)
		{
			archive(AmbientColor.r, AmbientColor.g, AmbientColor.b, IBLStrength, Enabled);
		}
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

		friend class cereal::access;

		template<typename Archive>
		void serialize(Archive& archive)
		{
			archive(Threshold, Knee, UpsampleScale, Intensity, DirtIntensity, Exposure, SkyboxMod, Enabled);
		}
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

		friend class cereal::access;

		template<typename Archive>
		void serialize(Archive& archive)
		{
			archive(EdgeThresholdMin, EdgeThresholdMax, Iterations, SubPixelQuality, InverseScreenSize.x, InverseScreenSize.y, Enabled);
		}
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

	struct DrawPackage
	{
		uint32_t    Instances = 0;
		uint32_t    Offset = 0;

		void Reset()
		{
			Instances = 0;
			Offset = 0;
		}
	};

	struct RendererDrawCommand
	{
		Ref<Mesh> Mesh = nullptr;
		std::map<Material3D*, DrawPackage> Packages;
	};

	struct ObjectData
	{
		glm::vec3* WorldPos = nullptr;
		glm::vec3* Rotation = nullptr;
		glm::vec3* Scale = nullptr;
		AnimationController* AnimController = nullptr;
		PBRHandle* PBRHandle = nullptr;

		void Reset()
		{
			WorldPos = nullptr;
			Rotation = nullptr;
			Scale = nullptr;
			PBRHandle = nullptr;
			AnimController = nullptr;
		}
	};

	struct RendererDrawInstance
	{
		struct PackageStorage
		{
			uint32_t                  Index = 0;
			std::vector<ObjectData>   Objects;
		};

		std::map<Ref<Mesh>, PackageStorage> Instances;
	};

	struct RendererDrawList
	{
		RendererDrawList();
		~RendererDrawList();

		static void              BeginSubmit(SceneViewProjection* sceneViewProj);
		static void              EndSubmit();
		static void              SubmitMesh(const glm::vec3& pos, const glm::vec3& rotation, const glm::vec3& scale, const Ref<Mesh>& mesh, const Ref<MeshView>& view);
		static void              SubmitDirLight(DirectionalLight* light);
		static void              SubmitPointLight(PointLight* light);
		static void              SubmitSpotLight(SpotLight* light);
		static void              CalculateFrustum(SceneViewProjection* viewProj);
		static void              SetDefaultState();
		static void              ClearDrawList();
		static void              ClearCache();
		static Frustum&          GetFrustum();
		static RendererDrawList* GetSingleton() { return s_Instance; }

	private:
		static void              CalculateDepthMVP();
		static void              BuildDrawList();

	private:
		inline static RendererDrawList*         s_Instance = nullptr;
		SceneViewProjection*                    m_SceneInfo = nullptr;
		uint32_t                                m_InstanceIndex = 0;
		uint32_t                                m_Objects = 0;
		uint32_t                                m_PointLightIndex = 0;
		uint32_t                                m_SpotLightIndex = 0;
		uint32_t                                m_LastAnimationOffset = 0;
		BoundingBox                             m_SceneAABB{};
											    
		Frustum                                 m_Frustum{};
		DirectionalLight                        m_DirLight{}; 
		glm::vec3                               m_MinPoint = glm::vec3(0);
		glm::mat4                               m_DepthMVP{};
		std::vector<RendererDrawCommand>        m_DrawList;
		std::array<InstanceData, max_objects>   m_InstancesData;
		std::array<PointLight, max_lights>      m_PointLights;
		std::array<SpotLight, max_lights>       m_SpotLights;
		std::vector<glm::mat4>                  m_AnimationJoints;

		std::map<Material3D*, RendererDrawInstance> m_Packages;
		std::unordered_map<Ref<Mesh>, uint32_t>     m_RootOffsets;

		friend struct RendererStorage;
		friend class RendererDeferred;
	};
}