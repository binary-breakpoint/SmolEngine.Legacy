#pragma once
#ifndef FROSTIUM_OPENGL_IMPL

#include "Primitives/GraphicsPipeline.h"
#include "Primitives/VertexBuffer.h"
#include "Primitives/IndexBuffer.h"
#include "Primitives/Texture.h"

#include "Window/Events.h"
#include "GUI/Backends/ContextBaseGUI.h"

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
#ifndef NK_GLFW_TEXT_MAX
#define NK_GLFW_TEXT_MAX 256
#endif

	class VulkanSwapchain;
	class Framebuffer;

	struct NuklearStorage;
	struct nk_context;

	class NuklearVulkanImpl: public ContextBaseGUI
	{
	public:
		void                             Init() override;
		void                             ShutDown() override;
		void                             NewFrame() override;
		void                             Draw(Framebuffer* target) override;
		void                             OnEvent(Event& e)override;
		nk_context*                      GetContext();
		
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