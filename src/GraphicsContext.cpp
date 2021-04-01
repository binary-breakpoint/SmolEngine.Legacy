#include "stdafx.h"
#include "GraphicsContext.h"

#include "Renderer.h"
#include "Renderer2D.h"
#include "Common/Framebuffer.h"
#include "Common/Window.h"

#include <GLFW/glfw3.h>

namespace Frostium
{
	GraphicsContext* GraphicsContext::s_Instance = new GraphicsContext();

	bool GraphicsContext::Init(GraphicsContextInitInfo* info)
	{
		if (s_Instance->m_Initialized || !info->WindowInfo)
			return false;

		s_Instance->m_Window.Init(info->WindowInfo);
		GLFWwindow* window = s_Instance->m_Window.GetNativeWindow();
#ifdef  FROSTIUM_OPENGL_IMPL
		s_Instance->m_OpenglContext.Setup(window);
#else
		s_Instance->m_VulkanContext.Setup(window,
			&s_Instance->m_Window.GetWindowData()->Width, &s_Instance->m_Window.GetWindowData()->Height);
#endif

		FramebufferSpecification framebufferCI = {};
		{
			framebufferCI.Width = info->Width;
			framebufferCI.Height = info->Height;
			framebufferCI.bUseMSAA = info->bMSAA;
			framebufferCI.bTargetsSwapchain = info->bTargetsSwapchain;
			framebufferCI.bUsedByImGui = true;
			framebufferCI.Attachments = { FramebufferAttachment(AttachmentFormat::Color) };

			Framebuffer::Create(framebufferCI, &s_Instance->m_Framebuffer);
		}

		Renderer::Init();
		Renderer2D::Init();
#ifdef  FROSTIUM_OPENGL_IMPL
		s_Instance->GetOpenglRendererAPI()->Init();
#endif
		s_Instance->m_Initialized = true;
		return true;
	}

	void GraphicsContext::OnResize(uint32_t height, uint32_t width)
	{
#ifdef  FROSTIUM_OPENGL_IMPL
		m_RendererAPI->SetViewport(0, 0, width, height);
#else
		s_Instance->m_VulkanContext.OnResize(height, width);
#endif
		s_Instance->m_Framebuffer.OnResize(width, height);
	}

	Framebuffer* GraphicsContext::GetFramebuffer()
	{
		return &s_Instance->m_Framebuffer;
	}

	GLFWwindow* GraphicsContext::GetNativeWindow()
	{
		return s_Instance->m_Window.GetNativeWindow();
}

	GraphicsContext* GraphicsContext::GetSingleton()
	{
		return s_Instance;
	}

	WindowData* GraphicsContext::GetWindowData()
	{
		return s_Instance->m_Window.GetWindowData();
	}

	void GraphicsContext::SwapBuffers()
	{
#ifdef  FROSTIUM_OPENGL_IMPL
		s_Instance->m_OpenglContext.SwapBuffers();
#else
		s_Instance->m_VulkanContext.SwapBuffers();
#endif
	}

	void GraphicsContext::BeginFrame()
	{
#ifdef  FROSTIUM_OPENGL_IMPL
#else
		s_Instance->m_VulkanContext.BeginFrame();
#endif
	}

	void GraphicsContext::ShutDown()
	{
		s_Instance->m_Window.ShutDown();
#ifdef FROSTIUM_OPENGL_IMPL
#else
		s_Instance->m_VulkanContext.~VulkanContext();
#endif
	}

#ifdef FROSTIUM_OPENGL_IMPL
	OpenglRendererAPI* GraphicsContext::GetOpenglRendererAPI()
	{
		return nullptr;
	}
#else
	VulkanContext& GraphicsContext::GetVulkanContext()
	{
		return s_Instance->m_VulkanContext;
	}
#endif
}