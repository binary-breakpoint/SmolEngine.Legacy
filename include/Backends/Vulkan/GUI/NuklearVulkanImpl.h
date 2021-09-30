#pragma once
#ifndef OPENGL_IMPL

#include "Primitives/GraphicsPipeline.h"
#include "Primitives/VertexBuffer.h"
#include "Primitives/IndexBuffer.h"
#include "Primitives/Texture.h"

#include "GUI/Backends/ContextBaseGUI.h"

namespace SmolEngine
{
#ifndef NK_GLFW_TEXT_MAX
#define NK_GLFW_TEXT_MAX 256
#endif

	class VulkanSwapchain;
	class Framebuffer;

	struct NuklearStorage;
	struct nk_context;
	struct nk_font;
	struct nk_font_atlas;

	class NuklearVulkanImpl: public ContextBaseGUI
	{
	public:
		void                             Init() override;
		void                             ShutDown() override;
		void                             NewFrame() override;
		void                             Draw(Framebuffer* target) override;
		void                             OnEvent(Event& e)override;
		void                             UpdateAtlas();
		void                             ClearAtlas();
		// Getters
		nk_context*                      GetContext();
		nk_font*                         GetFont();
		nk_font_atlas*                   GetFontAtlas();
		
		GraphicsPipeline                 m_Pipeline;
		Ref<VertexBuffer>                m_VertexBuffer;
		Ref<IndexBuffer>                 m_IndexBuffer;
		NuklearStorage*                  m_Info = nullptr;
		inline static NuklearVulkanImpl* s_Instance = nullptr;

	private:
		void                             SetupPipeline();
		void                             SetupBuffers();
	};
}

#endif