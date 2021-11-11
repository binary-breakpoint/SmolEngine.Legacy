#pragma once
#ifndef OPENGL_IMPL

#include "Graphics/Backends/Vulkan/VulkanVertexBuffer.h"
#include "Graphics/Backends/Vulkan/VulkanIndexBuffer.h"

#include "Graphics/Backends/Vulkan/VulkanPipeline.h"
#include "Graphics/Backends/Vulkan/VulkanTexture.h"

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

	class NuklearVulkanImpl
	{
	public:

		void                             Init();
		void                             ShutDown();
		void                             NewFrame();
		void                             Draw(Ref<Framebuffer>& target);
		void                             OnEvent(Event& e);
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