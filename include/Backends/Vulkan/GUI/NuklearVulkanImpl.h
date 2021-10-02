#pragma once
#ifndef OPENGL_IMPL

#include "Backends/Vulkan/VulkanVertexBuffer.h"
#include "Backends/Vulkan/VulkanIndexBuffer.h"

#include "Backends/Vulkan/VulkanPipeline.h"
#include "Backends/Vulkan/VulkanTexture.h"
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
		void                             Draw(Ref<Framebuffer>& target) override;
		void                             OnEvent(Event& e)override;
		void                             UpdateAtlas();
		void                             ClearAtlas();
		// Getters
		nk_context*                      GetContext();
		nk_font*                         GetFont();
		nk_font_atlas*                   GetFontAtlas();
		
		Ref<GraphicsPipeline>            m_Pipeline = nullptr;
		Ref<VertexBuffer>                m_VertexBuffer = nullptr;
		Ref<IndexBuffer>                 m_IndexBuffer = nullptr;
		NuklearStorage*                  m_Info = nullptr;
		inline static NuklearVulkanImpl* s_Instance = nullptr;

	private:
		void                             SetupPipeline();
		void                             SetupBuffers();
	};
}

#endif