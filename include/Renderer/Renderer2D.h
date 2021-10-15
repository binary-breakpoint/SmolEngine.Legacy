#pragma once
#include "Renderer/RendererShared.h"

#include "Primitives/GraphicsPipeline.h"
#include "Primitives/Framebuffer.h"
#include "Primitives/Mesh.h"

#include "Camera/Frustum.h"
#include "Tools/GLM.h"

namespace SmolEngine
{
	class Texture;
	struct ClearInfo;
	struct SceneViewProjection;
	struct CommandBufferStorage;

	struct Instance
	{
		glm::vec3*   Position = nullptr;
		glm::vec3*   Rotation = nullptr;
		glm::vec3*   Scale = nullptr;
		glm::vec4*   Color = nullptr;
		uint32_t*    Layer = nullptr;
		uint32_t     TextureIndex = 0;
	};

	struct CmdBuffer
	{
		bool         OffsetApplied = false;
		uint32_t     Instances = 0;
		uint32_t     DataOffset = 0;

		void Reset()
		{
			DataOffset = 0;
			Instances = 0;
			OffsetApplied = 0;
		}
	};

	struct ShaderInstance
	{
		glm::mat4    Model;
		glm::vec4    Color;
		glm::ivec4   Params; // x - texture index
	};

	struct RendererDrawList2D
	{
		RendererDrawList2D();
		~RendererDrawList2D();

		static void                       BeginSubmit(SceneViewProjection* viewProj);
		static void                       EndSubmit();
		static void                       SubmitSprite(const glm::vec3& worldPos, const glm::vec3& scale, const glm::vec3& rotation, uint32_t layerIndex, Ref<Texture>& texture,  bool frustumCulling = true, const glm::vec4& color = glm::vec4(1.0f));
		static void                       SubmitQuad(const glm::vec3& worldPos, const glm::vec3& scale, const glm::vec3& rotation, uint32_t layerIndex, bool frustumCulling = true, const glm::vec4& color = glm::vec4(1.0f));
		static void                       SubmitLight2D(const glm::vec3& worldPos, const glm::vec4& color, float radius, float lightIntensity, bool frustumCulling = true);
		static void                       CalculateFrustum(SceneViewProjection* viewProj);
		static Frustum&                   GetFrustum();
		static void                       ClearDrawList();
		static RendererDrawList2D*        GetSingleton() { return s_Instance; }
								          
	private:					          
		void                              BuildDrawList();
		bool                              IsDrawable(const glm::vec3& worldPos, bool frustumCulling);
		uint32_t                          AddTexture(Ref<Texture>& tex);

	private:
		inline static RendererDrawList2D* s_Instance = nullptr;
		static const uint32_t             Light2DBufferMaxSize = 100;
		static const uint32_t             MaxQuads = 15000;
		static const uint32_t             MaxTextMessages = 100;
		static const uint32_t             MaxLayers = 12;
		static const uint32_t             MaxTextureSlot = 4096;
								          
		SceneViewProjection*              SceneInfo = nullptr;
								          
		uint32_t                          InstIndex = 0;
		uint32_t                          TextIndex = 1;
		uint32_t                          SampleIndex = 1; // 0 - white dummy texture
		uint32_t                          FontSampleIndex = 0;
								          
		Frustum                           Frustum{};
		ShaderInstance                    ShaderInstances[MaxQuads];
		Instance                          Instances[MaxQuads];
		CmdBuffer                         CommandBuffer[MaxLayers];
		std::vector<Ref<Texture>>         Textures;
		std::vector<Ref<Texture>>         FontTextures;

		friend struct Renderer2DStorage;
		friend class Renderer2D;
	};

	struct Renderer2DStorage: RendererStorageBase
	{
		Renderer2DStorage();
		~Renderer2DStorage();

		static void                      SetRenderTarget(Ref<Framebuffer>& target);
		static Renderer2DStorage*        GetSingleton() { return s_Instance; }
										 
	private:							 
		void                             Initilize() override;
		void                             CreatePipelines();
		void                             CreateFramebuffers();
		void                             UpdateUniforms(RendererDrawList2D* drawList);

	private:
		inline static Renderer2DStorage* s_Instance = nullptr;
		Ref<GraphicsPipeline>            MainPipeline = nullptr;
		Ref<Framebuffer>                 MainFB = nullptr;
		Ref<Mesh>                        PlaneMesh = nullptr;
		const uint32_t                   InstancesBP = 1;
		const uint32_t                   SamplersBP = 2;
		const uint32_t                   SceneDataBP = 27;

		struct TextPC
		{
			glm::mat4 Model;
			glm::vec4 Color;
			uint32_t TexIndex;

		} TextPushConstant;

		const size_t ShaderInstanceSize = sizeof(ShaderInstance);
		const size_t TextPCSize = sizeof(TextPC);

		friend class Renderer2D;
		friend class Material;
		friend class GraphicsContext;
	};

	class Renderer2D
	{
	public:
		static void DrawFrame(ClearInfo* clearInfo, bool batch_cmd = true);

	private:
		static void PrepareCmdBuffer(CommandBufferStorage* cmd, Renderer2DStorage* storage, bool batch_cmd);
		static void ClearAtachments(const ClearInfo* clearInfo, Renderer2DStorage* storage);
		static void UpdateCmdBuffer(Renderer2DStorage* storage, RendererDrawList2D* drawList);
		static void UpdateUniforms(Renderer2DStorage* storage, RendererDrawList2D* drawList);
	};
}