#pragma once

#include "Common/RendererShared.h"

namespace Frostium
{
	class Framebuffer;

	class Renderer2D
	{
	public:

		static void Init();

		static void Shutdown();

		static void BeginScene(const ClearInfo* clearInfo, const BeginSceneInfo* info = nullptr);

		static void EndScene();

		// Submit

		static void SubmitSprite(const glm::vec3& worldPos, const uint32_t layerIndex, const glm::vec2& scale,
			const float rotation, const Ref<Texture>& texture);

		static void SubmitQuad(const glm::vec3& worldPos, const uint32_t layerIndex,
			const glm::vec2& scale, const float rotation, const glm::vec4& color);

		static void SubmitText(const glm::vec3& pos, const glm::vec2& scale,
			const Ref<Texture> texture, const glm::vec4& tintColor = glm::vec4(1.0f));

		static void SubmitLight2D(const glm::vec3& offset, const float radius, const glm::vec4& color, const float lightIntensity);


	private:

		// Helpers

		static void CreatePipelines();

		static void CreateFramebuffers();

		static void StartNewBatch();

		static void UploadLightUniforms();

	public:

#ifdef FROSTIUM_OPENGL_IMPL
		static const uint32_t MaxTextureSlot = 32;
#else
		static const uint32_t MaxTextureSlot = 4096;
#endif

	private:

		static Renderer2DStats* Stats;
		inline static bool m_RenderResetted = false;
	};
}