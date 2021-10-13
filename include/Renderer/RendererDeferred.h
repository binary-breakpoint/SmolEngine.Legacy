#pragma once
#include "Renderer/RendererShared.h"

#include "Primitives/GraphicsPipeline.h"
#include "Primitives/ComputePipeline.h"
#include "Primitives/Framebuffer.h"
#include "Primitives/Texture.h"
#include "Primitives/Mesh.h"
#include "Primitives/EnvironmentMap.h"

#include "Material/MaterialPBR.h"

#include "Animation/AnimationController.h"
#include "Camera/Frustum.h"
#include "Tools/GLM.h"

namespace SmolEngine
{
	struct CommandBuffer
	{
		uint32_t               InstancesCount = 0;
		uint32_t               Offset = 0;
		Ref<Mesh>              Mesh = nullptr;
		Material*              Material = nullptr;
	};

	struct InstancePackage
	{
		struct Package
		{
			glm::vec3*           WorldPos = nullptr;
			glm::vec3*           Rotation = nullptr;
			glm::vec3*           Scale = nullptr;
			AnimationController* AnimController = nullptr;
			PBRHandle*           PBRHandle = nullptr;
			Material*            Material = nullptr;
		};

		uint32_t                  CurrentIndex = 0;
		std::vector<Package>      Packages;
	};

	struct RendererDrawList
	{
		RendererDrawList();

		void                                            BeginSubmit(SceneViewProjection* sceneViewProj);
		void                                            EndSubmit();				    
		void                                            SubmitMesh(const glm::vec3& pos, const glm::vec3& rotation, const glm::vec3& scale, const Ref<Mesh>& mesh, const Ref<PBRHandle>& pbr_handle = nullptr,
			                                            bool submit_childs = true, AnimationController* anim_controller = nullptr);
		void                                            SubmitDirLight(DirectionalLight* light);
		void                                            SubmitPointLight(PointLight* light);
		void                                            SubmitSpotLight(SpotLight* light);    
		void                                            CalculateFrustum(SceneViewProjection* viewProj);
		void                                            SetDefaultState();
		Frustum&                                        GetFrustum();
													    
	private:										    
		void                                            CalculateDepthMVP();
		void                                            BuildDrawList();
		void                                            ResetDrawList();

	private:
		SceneViewProjection*                            m_SceneInfo = nullptr;
													    
		uint32_t                                        m_Objects = 0;
		uint32_t                                        m_InstanceDataIndex = 0;
		uint32_t                                        m_PointLightIndex = 0;
		uint32_t                                        m_SpotLightIndex = 0;
		uint32_t                                        m_LastAnimationOffset = 0;

		Frustum                                         m_Frustum{};
		DirectionalLight                                m_DirLight{};
		glm::mat4                                       m_DepthMVP{};
		std::vector<Ref<Mesh>>                          m_UsedMeshes;
		std::vector<CommandBuffer>                      m_DrawList;
		std::array<InstanceData, max_objects>           m_InstancesData;
		std::array<PointLight, max_lights>              m_PointLights;
		std::array<SpotLight, max_lights>               m_SpotLights;
		std::vector<glm::mat4>                          m_AnimationJoints;
		std::unordered_map<Ref<Mesh>, InstancePackage>  m_Packages;
		std::unordered_map<Ref<Mesh>, uint32_t>         m_RootOffsets;

		friend struct RendererStorage;
		friend class RendererDeferred;
	};

	struct RendererStorage: RendererStorageBase
	{
		void                          SetDynamicSkybox(DynamicSkyProperties& properties, const glm::mat4& proj, bool regeneratePBRmaps);
		void                          SetStaticSkybox(Ref<Texture>& skybox);
		void                          SetRenderTarget(Ref<Framebuffer>& target);
		void                          SetDefaultState();
		RendererStateEX&              GetState();
		Ref<MaterialPBR>              GetDefaultMaterial() const;
		void                          OnResize(uint32_t width, uint32_t height) override;
		void                          OnUpdateMaterials();

	private:
		void                          Initilize() override;
		void                          CreatePipelines();
		void                          CreateFramebuffers();
		void                          CreatePBRMaps();
		void                          UpdateUniforms(RendererDrawList* drawList, Ref<Framebuffer>& target);

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
		const uint32_t                m_BloomComputeWorkgroupSize = 4;
		// Materials
		Ref<MaterialPBR>              m_DefaultMaterial = nullptr;
		// Pipelines				
		Ref<GraphicsPipeline>         p_Lighting = nullptr;
		Ref<GraphicsPipeline>         p_Combination = nullptr;
		Ref<GraphicsPipeline>         p_Skybox = nullptr;
		Ref<GraphicsPipeline>         p_DepthPass = nullptr;
		Ref<GraphicsPipeline>         p_Grid = nullptr;
		Ref<GraphicsPipeline>         p_Debug = nullptr;
		Ref<GraphicsPipeline>         p_Mask = nullptr;
		Ref<GraphicsPipeline>         p_DOF = nullptr;
		Ref<ComputePipeline>          p_Bloom = nullptr;
		// Framebuffers				
		Ref<Framebuffer>              f_Main = nullptr;
		Ref<Framebuffer>              f_GBuffer = nullptr;
		Ref<Framebuffer>              f_Lighting = nullptr;
		Ref<Framebuffer>              f_Depth = nullptr;
		Ref<Framebuffer>              f_DOF = nullptr;
		Ref<Mesh>                     m_GridMesh = nullptr;
		Ref<VulkanPBR>                m_VulkanPBR = nullptr;
		Ref<EnvironmentMap>           m_EnvironmentMap = nullptr;

		RendererStateEX               m_State{};
		std::vector<Ref<Texture>>     m_BloomTex{};
		ShadowMapSize                 m_MapSize = ShadowMapSize::SIZE_8;
		glm::mat4                     m_GridModel{};
												    
		friend class RendererDeferred;
		friend class Material;
		friend class MaterialPBR;
	};

	class RendererDeferred
	{
		struct SubmitInfo
		{
			ClearInfo*            pClearInfo = nullptr;
			RendererStorage*      pStorage = nullptr;
			RendererDrawList*     pDrawList = nullptr;
			CommandBufferStorage* pCmdStorage = nullptr;
		};
	public:
		static void DrawFrame(ClearInfo* clearInfo, RendererStorage* storage, RendererDrawList* drawList, bool batch_cmd = true);

	private:
		static void GBufferPass(SubmitInfo* info);
		static void LightingPass(SubmitInfo* info);
		static void DepthPass(SubmitInfo* info);
		static void BloomPass(SubmitInfo* info);
		static bool DebugViewPass(SubmitInfo* info);
		static void CompositionPass(SubmitInfo* info);
		static void UpdateUniforms(SubmitInfo* info);
	};
}
