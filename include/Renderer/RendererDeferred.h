#pragma once
#include "Primitives/GraphicsPipeline.h"
#include "Primitives/Framebuffer.h"
#include "Primitives/Texture.h"
#include "Primitives/Mesh.h"

#include "Environment/EnvironmentMap.h"
#include "Animation/AnimationController.h"
#include "Tools/MaterialLibrary.h"
#include "Camera/Frustum.h"
#include "Tools/GLM.h"

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
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
		bool           IsCastShadows = true;
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
		float          Exposure = 1.0f;
		float          Threshold = 0.10f;
		float          Scale = 0.5f;
		float          Strength = 0.5f;
		bool           Enabled = false;
	private:
		GLSL_BOOLPAD   Pad1;
		uint32_t       Pad2;
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

	struct CommandBuffer
	{
		uint32_t               InstancesCount = 0;
		uint32_t               Offset = 0;
		Mesh* Mesh = nullptr;
	};

	struct InstancePackage
	{
		struct Package
		{
			uint32_t              MaterialID = 0;
			glm::vec3* WorldPos = nullptr;
			glm::vec3* Rotation = nullptr;
			glm::vec3* Scale = nullptr;
			AnimationController* AnimController = nullptr;
		};

		uint32_t                  CurrentIndex = 0;
		std::vector<Package>      Packages;
	};

	struct RendererDrawList
	{
		RendererDrawList();

		void                                        BeginSubmit(SceneViewProjection* sceneViewProj);
		void                                        EndSubmit();

		void                                        SubmitMesh(const glm::vec3& pos, const glm::vec3& rotation, const glm::vec3& scale, Mesh* mesh, const uint32_t& material_id = 0, bool submit_childs = true, AnimationController* anim_controller = nullptr);
		void                                        SubmitDirLight(DirectionalLight* light);
		void                                        SubmitPointLight(PointLight* light);
		void                                        SubmitSpotLight(SpotLight* light);

		void                                        SetViewProjection(SceneViewProjection* sceneViewProj);
		void                                        SetDefaultState();
		Frustum&                                    GetFrustum();

	private:
		void                                        CalculateDepthMVP();
		void                                        BuildDrawList();
		void                                        ResetDrawList();

	private:
		SceneViewProjection*                        m_SceneInfo = nullptr;

		uint32_t                                    m_Objects = 0;
		uint32_t                                    m_InstanceDataIndex = 0;
		uint32_t                                    m_PointLightIndex = 0;
		uint32_t                                    m_SpotLightIndex = 0;
		uint32_t                                    m_LastAnimationOffset = 0;

		Frustum                                     m_Frustum{};
		DirectionalLight                            m_DirLight{};
		glm::mat4                                   m_DepthMVP{};
		std::vector<Mesh*>                          m_UsedMeshes;
		std::vector<CommandBuffer>                  m_DrawList;
		std::array<InstanceData, max_objects>       m_InstancesData;
		std::array<PointLight, max_lights>          m_PointLights;
		std::array<SpotLight, max_lights>           m_SpotLights;
		std::vector<glm::mat4>                      m_AnimationJoints;
		std::unordered_map<Mesh*, InstancePackage>  m_Packages;
		std::unordered_map<Mesh*, uint32_t>         m_RootOffsets;

		friend struct RendererStorage;
		friend class RendererDeferred;
	};

	struct RendererStorage: RendererStorageBase
	{
		void                          Initilize() override;

		void                          SetDynamicSkybox(DynamicSkyProperties& properties, const glm::mat4& proj, bool regeneratePBRmaps);
		void                          SetStaticSkybox(CubeMap* cube);
		void                          SetRenderTarget(Framebuffer* target);
		void                          SetDefaultState();

		RendererStateEX&              GetState();
		MaterialLibrary&              GetMaterialLibrary();

		void                          OnResize(uint32_t width, uint32_t height) override;
		void                          OnUpdateMaterials();

	private:
		void                          CreatePipelines();
		void                          CreateFramebuffers();
		void                          CreatePBRMaps();
		void                          UpdateUniforms(RendererDrawList* drawList, Framebuffer* target);

	private:
		// Bindings					
		const uint32_t                m_TexturesBinding = 24;
		const uint32_t                m_ShaderDataBinding = 25;
		const uint32_t                m_MaterialsBinding = 26;
		const uint32_t                m_SceneDataBinding = 27;
		const uint32_t                m_AnimBinding = 28;
		const uint32_t                m_PointLightBinding = 30;
		const uint32_t                m_SpotLightBinding = 31;
		const uint32_t                m_DirLightBinding = 32;
		const uint32_t                m_LightingStateBinding = 33;
		const uint32_t                m_BloomStateBinding = 34;
		const uint32_t                m_FXAAStateBinding = 35;
		const uint32_t                m_DynamicSkyBinding = 36;
		// Pipelines				
		GraphicsPipeline              p_Gbuffer = {};
		GraphicsPipeline              p_Lighting = {};
		GraphicsPipeline              p_Bloom = {};
		GraphicsPipeline              p_Combination = {};
		GraphicsPipeline              p_Skybox = {};
		GraphicsPipeline              p_DepthPass = {};
		GraphicsPipeline              p_Grid = {};
		GraphicsPipeline              p_Debug = {};
		GraphicsPipeline              p_Mask = {};
		// Framebuffers				
		Framebuffer*                  f_Main = nullptr;
		Framebuffer                   f_GBuffer = {};
		Framebuffer                   f_Lighting = {};
		Framebuffer                   f_Bloom = {};
		Framebuffer                   f_Depth = {};
				            
		Mesh                          m_PlaneMesh = {};
		MaterialLibrary               m_MaterialLibrary{};
		RendererStateEX               m_State{};
		struct PushConstant
		{
			glm::mat4                 DepthMVP = glm::mat4(1.0f);
			uint32_t                  DataOffset = 0;
		};							
									
		ShadowMapSize                 m_MapSize = ShadowMapSize::SIZE_8;
		glm::mat4                     m_GridModel{};
		PushConstant                  m_MainPushConstant = {};
		VulkanPBR*                    m_VulkanPBR = nullptr;
		Ref<EnvironmentMap>           m_EnvironmentMap = nullptr;
		// Sizes					
		const size_t                  m_PushConstantSize = sizeof(PushConstant);
												    
		friend class RendererDeferred;
		friend class MaterialLibrary;
	};

	class RendererDeferred
	{
	public:
		static void DrawFrame(ClearInfo* clearInfo, RendererStorage* storage, RendererDrawList* drawList, bool batch_cmd = true);

	private:
		static void PrepareCmdBuffer(CommandBufferStorage* cmdStorage, RendererStorage* rendererStorage, bool batch);
		static void ClearAtachments(ClearInfo* clearInfo, RendererStorage* storage);
		static void UpdateCmdBuffer(RendererStorage* storage, RendererDrawList* drawList);
		static void UpdateUniforms(RendererStorage* storage, RendererDrawList* drawList);
	};
}
