#pragma once
#include "Common/RendererShared.h"

namespace Frostium
{
	class GraphicsPipeline;
	class Renderer2D
	{
	public:

		static void Init();
		static void Shutdown();
		static void BeginScene(const ClearInfo* clearInfo, const BeginSceneInfo* info = nullptr);
		static void EndScene();

		// Submit
		static void SubmitSprite(const glm::vec3& worldPos, const glm::vec2& scale, const glm::vec4& color,
			float rotation, uint32_t layerIndex, Texture* texture, GraphicsPipeline* material = nullptr);
		static void SubmitQuad(const glm::vec3& worldPos, const glm::vec2& scale, 
			const glm::vec4& color, float rotation, uint32_t layerIndex, GraphicsPipeline* material = nullptr);
		static void SubmitText(const glm::vec3& pos, const glm::vec2& scale,
			Texture* texture, const glm::vec4& color = glm::vec4(1.0f));
		static void SubmitLight2D(const glm::vec3& worldPos, const glm::vec4& color, 
			float radius, float lightIntensity);

	private:

		static void Flush();
		static void StartNewBatch();
		static void Reset();

		static uint32_t AddTexture(Texture* tex);

		static void CreatePipelines();
		static void CreateFramebuffers();
		static void Prepare();

	public:

#ifdef FROSTIUM_OPENGL_IMPL
		static const uint32_t MaxTextureSlot = 32;
#else
		static const uint32_t MaxTextureSlot = 4096;
#endif

	private:

		inline static Renderer2DStats* Stats = nullptr;
		inline static bool m_RenderResetted = false;
	};
}