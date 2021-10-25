#pragma once
#include "Renderer/RendererShared.h"

namespace SmolEngine
{
	class MaterialPBR;
	class PBRFactory;
	class PBRLoader;

	struct SubmitInfo;

	struct RendererStorage : RendererStorageBase
	{
		RendererStorage();
		~RendererStorage();

		static void                    SetDynamicSkybox(DynamicSkyProperties& properties, const glm::mat4& proj, bool regeneratePBRmaps);
		static void                    SetStaticSkybox(Ref<Texture>& skybox);
		static void                    SetRenderTarget(Ref<Framebuffer>& target);
		static void                    SetDefaultState();
		static RendererStateEX&        GetState();
		static Ref<MaterialPBR>        GetDefaultMaterial();
		static Ref<PBRLoader>          GetPBRLoader();
		static RendererStorage*        GetSingleton() { return s_Instance; }
									   
	private:						   
		void                           Initilize() override;
		void                           CreatePipelines();
		void                           CreateFramebuffers();
		void                           CreatePBRMaps();
		void                           UpdateUniforms(RendererDrawList* drawList, Ref<Framebuffer>& target);
		void                           OnResize(uint32_t width, uint32_t height) override;

	private:
		inline static RendererStorage* s_Instance = nullptr;
		// Bindings					   
		const uint32_t                 m_TexturesBinding = 24;
		const uint32_t                 m_ShaderDataBinding = 25;
		const uint32_t                 m_MaterialsBinding = 26;
		const uint32_t                 m_SceneDataBinding = 27;
		const uint32_t                 m_AnimBinding = 28;
		const uint32_t                 m_PointLightBinding = 30;
		const uint32_t                 m_SpotLightBinding = 31;
		const uint32_t                 m_DirLightBinding = 32;
		const uint32_t                 m_LightingStateBinding = 33;
		const uint32_t                 m_BloomStateBinding = 34;
		const uint32_t                 m_FXAAStateBinding = 35;
		const uint32_t                 m_DynamicSkyBinding = 36;
		const uint32_t                 m_BloomComputeWorkgroupSize = 4;
		// Materials				   
		Ref<MaterialPBR>               m_DefaultMaterial = nullptr;
		// Pipelines				   
		Ref<GraphicsPipeline>          p_Lighting = nullptr;
		Ref<GraphicsPipeline>          p_Combination = nullptr;
		Ref<GraphicsPipeline>          p_Skybox = nullptr;
		Ref<GraphicsPipeline>          p_DepthPass = nullptr;
		Ref<GraphicsPipeline>          p_Grid = nullptr;
		Ref<GraphicsPipeline>          p_Debug = nullptr;
		Ref<GraphicsPipeline>          p_Mask = nullptr;
		Ref<GraphicsPipeline>          p_DOF = nullptr;
		Ref<ComputePipeline>           p_Bloom = nullptr;
		// Framebuffers				   
		Ref<Framebuffer>               f_Main = nullptr;
		Ref<Framebuffer>               f_GBuffer = nullptr;
		Ref<Framebuffer>               f_Lighting = nullptr;
		Ref<Framebuffer>               f_Depth = nullptr;
		Ref<Framebuffer>               f_DOF = nullptr;
		Ref<Mesh>                      m_GridMesh = nullptr;
		Ref<PBRLoader>                 m_PBRLoader = nullptr;
		Ref<EnvironmentMap>            m_EnvironmentMap = nullptr;
		Ref<PBRFactory>                m_PBRFactory = nullptr;

		RendererStateEX                m_State{};
		std::vector<Ref<Texture>>      m_BloomTex{};
		ShadowMapSize                  m_MapSize = ShadowMapSize::SIZE_8;
		glm::mat4                      m_GridModel{};

		friend class Material;
		friend class PBRFactory;
		friend class MaterialPBR;
		friend class GraphicsContext;
		friend class RendererDeferred;
	};

	class RendererDeferred
	{
	public:
		static void DrawFrame(ClearInfo* clearInfo, bool batch_cmd = true);

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
