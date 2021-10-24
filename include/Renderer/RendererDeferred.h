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
		Ref<GraphicsPipeline>          p_Voxelization = nullptr;
		Ref<GraphicsPipeline>          p_VoxelView = nullptr;
		Ref<GraphicsPipeline>          p_GI = nullptr;
		Ref<ComputePipeline>           p_Bloom = nullptr;
		Ref<ComputePipeline>           p_InjectRadiance = nullptr;
		Ref<ComputePipeline>           p_InjectPropagation = nullptr;
		Ref<ComputePipeline>           p_MipmapBase = nullptr;
		Ref<ComputePipeline>           p_MipmapVolume = nullptr;
		// Framebuffers				   
		Ref<Framebuffer>               f_Main = nullptr;
		Ref<Framebuffer>               f_GBuffer = nullptr;
		Ref<Framebuffer>               f_Lighting = nullptr;
		Ref<Framebuffer>               f_Depth = nullptr;
		Ref<Framebuffer>               f_Voxelization = nullptr;
		Ref<Framebuffer>               f_DOF = nullptr;
		Ref<Mesh>                      m_GridMesh = nullptr;
		Ref<PBRLoader>                 m_PBRLoader = nullptr;
		Ref<EnvironmentMap>            m_EnvironmentMap = nullptr;
		Ref<PBRFactory>                m_PBRFactory = nullptr;

		struct VoxelConeTracing
		{
			uint32_t      VolumeDimension = 256;
			uint32_t      VoxelCount = 0;
			float         VolumeGridSize = 0.0f;
			float         VoxelSize = 0;
			// TEMP
			//glm::vec3     AxisSize = glm::vec3(1860.42700, 777.937866, 1144.11658);
			//glm::vec3     Center = glm::vec3(-60.5189209, 651.495361, -38.6905518);
			//glm::vec3     MinPoint = glm::vec3(-1920.94592, -126.442497, -1182.80713);
			//glm::vec3     MaxPoint = glm::vec3(1799.90808, 1429.43323, 1105.42603);

			Ref<Texture>  VoxelAlbedo = nullptr;
			Ref<Texture>  VoxelNormal = nullptr;
			Ref<Texture>  VoxelMaterials = nullptr;
			Ref<Texture>  AlbedoBuffer = nullptr;
			Ref<Texture>  NormalBuffer = nullptr;
			Ref<Texture>  MaterialsBuffer = nullptr;
			Ref<Texture>  VoxelFlags = nullptr;
			Ref<Texture>  Radiance = nullptr;
			std::vector<Ref<Texture>> VoxelTexMipmap;

			struct UBO
			{
				glm::mat4 viewProjections[3];
				glm::mat4 viewProjectionsI[3];
				glm::vec3 worldMinPoint;
				float voxelScale;
				uint32_t volumeDimension;
				uint32_t flagStaticVoxels;
				glm::vec2 pad;

			} ubo{};

			struct LightData
			{
				struct Attenuation
				{
					float constant = 1.0f;
					float linear = 0.2f;
					float quadratic = 0.080f;
					float pad = 0.0f;
				};

				float angleInnerCone = 25.0f;
				float angleOuterCone = 30.0f;
				uint32_t shadowingMethod = 1;
				glm::vec3 diffuse = glm::vec3(255, 255, 255);

				glm::vec3 position = glm::vec3(0, 0 , 0);
				float pad1 = 0.0f;
				glm::vec3 direction = glm::vec3(105.0f, 53.0f, 102.0f);
				float pad2 = 0.0f;
				Attenuation attenuation{};

			} dirLight{};
		};

		VoxelConeTracing               m_VCTParams{};
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
		static void VoxelizationPass(SubmitInfo* info);
		static bool DebugViewPass(SubmitInfo* info);
		static void CompositionPass(SubmitInfo* info);
		static void UpdateUniforms(SubmitInfo* info);
	};
}
