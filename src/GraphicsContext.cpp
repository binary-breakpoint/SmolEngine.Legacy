#include "stdafx.h"
#include "GraphicsContext.h"
#include "Renderer.h"
#include "Renderer2D.h"

#include "Common/SLog.h"
#include "Common/Input.h"
#include "Common/ApplicationEvent.h"

#include <GLFW/glfw3.h>

namespace Frostium
{
	GraphicsContext* GraphicsContext::s_Instance = new GraphicsContext();

	bool GraphicsContext::Init(GraphicsContextInitInfo* info)
	{
		if (s_Instance->m_Initialized || !info->WindowInfo)
			return false;

		SLog::InitLog();
		s_Instance->m_ResourcesFolderPath = info->ResourcesFolderPath;
		s_Instance->m_EventHandler.OnEventFn = std::bind(&GraphicsContext::OnEvent, s_Instance, std::placeholders::_1);
		s_Instance->m_Window.Init(info->WindowInfo, &s_Instance->m_EventHandler);
		GLFWwindow* window = s_Instance->m_Window.GetNativeWindow();
#ifdef  FROSTIUM_OPENGL_IMPL
		s_Instance->m_OpenglContext.Setup(window);
#else
		s_Instance->m_VulkanContext.Setup(window,
			&s_Instance->m_Window.GetWindowData()->Width, &s_Instance->m_Window.GetWindowData()->Height);
#endif
		s_Instance->m_DummyTexure = Texture::CreateWhiteTexture();
		FramebufferSpecification framebufferCI = {};
		{
			framebufferCI.Width = info->WindowInfo->Width;
			framebufferCI.Height = info->WindowInfo->Height;
			framebufferCI.bUseMSAA = info->bMSAA;
			framebufferCI.bTargetsSwapchain = info->bTargetsSwapchain;
			framebufferCI.bUsedByImGui = true;
			framebufferCI.Attachments = { FramebufferAttachment(AttachmentFormat::Color) };

			Framebuffer::Create(framebufferCI, &s_Instance->m_Framebuffer);
		}

		s_Instance->m_ImGuiContext.Init();
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

	void GraphicsContext::SetEventCallback(std::function<void(Event&)> callback)
	{
		s_Instance->m_EventCallback = callback;
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

	void GraphicsContext::OnEvent(Event& event)
	{
		m_ImGuiContext.OnEvent(event);
		if (Input::IsEventReceived(EventType::S_WINDOW_RESIZE, event))
		{
			auto& resize_event = static_cast<WindowResizeEvent&>(event);
			uint32_t width = resize_event.GetWidth();
			uint32_t height = resize_event.GetHeight();
#ifdef  FROSTIUM_OPENGL_IMPL
#else
			s_Instance->m_VulkanContext.OnResize(width, height);
#endif
			s_Instance->m_Framebuffer.OnResize(width, height);
		}

		m_EventCallback(std::forward<Event&>(event));
	}

	void GraphicsContext::SwapBuffers()
	{
		s_Instance->m_ImGuiContext.OnEnd();
#ifdef  FROSTIUM_OPENGL_IMPL
		s_Instance->m_OpenglContext.SwapBuffers();
#else
		s_Instance->m_VulkanContext.SwapBuffers();
#endif
	}

	void GraphicsContext::BeginFrame()
	{
		s_Instance->m_Window.ProcessEvents();
#ifdef  FROSTIUM_OPENGL_IMPL
#else
		s_Instance->m_VulkanContext.BeginFrame();
#endif
		s_Instance->m_ImGuiContext.OnBegin();
	}

	void GraphicsContext::ShutDown()
	{
		s_Instance->m_ImGuiContext.ShutDown();
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