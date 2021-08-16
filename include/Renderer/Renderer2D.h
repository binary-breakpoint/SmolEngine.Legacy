#pragma once

#include "Primitives/GraphicsPipeline.h"
#include "Primitives/Framebuffer.h"
#include "Primitives/Mesh.h"
#include "Primitives/Text.h"

#include "Camera/Frustum.h"
#include "Tools/GLM.h"

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
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
		glm::mat4  Model;
		glm::vec4  Color;
		glm::ivec4 Params; // x - texture index
	};

	struct TextBuffer
	{
		glm::mat4    Model;
		Text* Obj = nullptr;
		uint32_t     TextureIndex = 0;
	};

	struct Renderer2DStorage: RendererStorageBase
	{
		Renderer2DStorage();
		~Renderer2DStorage() = default;

		void                      Initilize() override;
		void                      BeginSubmit(SceneViewProjection* sceneViewProj) override;
		void                      EndSubmit() override;

		void                      SubmitSprite(const glm::vec3& worldPos, const glm::vec3& scale, const glm::vec3& rotation, uint32_t layerIndex, Texture* texture, const glm::vec4& color = glm::vec4(1.0f), GraphicsPipeline* material = nullptr);
		void                      SubmitQuad(const glm::vec3& worldPos, const glm::vec3& scale, const glm::vec3& rotation, uint32_t layerIndex, const glm::vec4& color = glm::vec4(1.0f), GraphicsPipeline* material = nullptr);
		void                      SubmitLight2D(const glm::vec3& worldPos, const glm::vec4& color, float radius, float lightIntensity);
		void                      SubmitText(Text* text);

		void                      SetRenderTarget(Framebuffer* target);
		void                      SetViewProjection(const SceneViewProjection* sceneViewProj);
		Frustum&                  GetFrustum();

	private:
		uint32_t                  AddTexture(Texture* tex);
		void                      CreatePipelines();
		void                      CreateFramebuffers();
		void                      ClearDrawList();
		void                      BuildDrawList();
		void                      UpdateUniforms(const SceneViewProjection* sceneViewProj);

	private:
		// Limits
		static const uint32_t     Light2DBufferMaxSize = 100;
		static const uint32_t     MaxQuads = 15000;
		static const uint32_t     MaxTextMessages = 100;
		static const uint32_t     MaxLayers = 12;
		static const uint32_t     MaxTextureSlot = 4096;
		// Bindings
		const uint32_t            InstancesBP = 1;
		const uint32_t            SamplersBP = 2;
		const uint32_t            SceneDataBP = 27;
		// Count tracking
		uint32_t                  InstIndex = 0;
		uint32_t                  TextIndex = 1;
		uint32_t                  SampleIndex = 1; // 0 - white dummy texture
		uint32_t                  FontSampleIndex = 0;
		Framebuffer*              MainFB = nullptr;
		GraphicsPipeline          MainPipeline = {};
		GraphicsPipeline          TextPipeline = {};
		Texture*                  WhiteTetxure = nullptr;
		SceneViewProjection*      SceneInfo = nullptr;
		Mesh                      PlaneMesh = {};
		Frustum                   m_Frustum{};
		// UBO's and SSBO's
		ShaderInstance            ShaderInstances[MaxQuads];
		TextBuffer                TextMessages[MaxTextMessages];
		// Buffers
		Instance                  Instances[MaxQuads];
		CmdBuffer                 CommandBuffer[MaxLayers];
		std::vector<Texture*>     Textures;
		std::vector<Texture*>     FontTextures;
		// Push Constants
		struct TextPC
		{
			glm::mat4             Model;
			glm::vec4             Color;
			uint32_t              TexIndex;
		};
		TextPC                    TextPushConstant;
		// Sizes
		const size_t              ShaderInstanceSize = sizeof(ShaderInstance);
		const size_t              TextPCSize = sizeof(TextPC);

		friend class Renderer2D;
	};

	class Renderer2D
	{
	public:
		static void DrawFrame(const ClearInfo* clearInfo, Renderer2DStorage* storage, bool batch_cmd = true);

	private:
		static void UpdateCmdBuffer(Renderer2DStorage* storage);
		static void PrepareCmdBuffer(CommandBufferStorage* cmd, Renderer2DStorage* storage, bool batch_cmd);
		static void ClearAtachments(const ClearInfo* clearInfo, Renderer2DStorage* storage);
	};
}