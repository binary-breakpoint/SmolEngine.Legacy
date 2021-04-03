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
	GraphicsContext* GraphicsContext::s_Instance = nullptr;

	GraphicsContext::GraphicsContext(GraphicsContextInitInfo* info)
		:
		m_UseImGUI(info->bImGUI),
		m_UseEditorCamera(info->EditorCameraCI != nullptr ? true: false)
	{
		if (m_Initialized || !info->WindowCI)
			return;

		s_Instance = this;
		// Initialize spdlog
		SLog::InitLog();
		m_ResourcesFolderPath = info->ResourcesFolderPath;
		m_EventHandler.OnEventFn = std::bind(&GraphicsContext::OnEvent, this, std::placeholders::_1);
		// Creates GLFW window
		m_Window.Init(info->WindowCI, &m_EventHandler);
		GLFWwindow* window = m_Window.GetNativeWindow();
		// Creates API context
#ifdef  FROSTIUM_OPENGL_IMPL
		m_OpenglContext.Setup(window);
#else
		m_VulkanContext.Setup(window,
			&m_Window.GetWindowData()->Width, &m_Window.GetWindowData()->Height);
#endif
		// Creates white 4x4 white texture
		m_DummyTexure = Texture::CreateWhiteTexture();
		// Creates main framebuffer
		FramebufferSpecification framebufferCI = {};
		{
			framebufferCI.Width = info->WindowCI->Width;
			framebufferCI.Height = info->WindowCI->Height;
			framebufferCI.bUseMSAA = info->bMSAA;
			framebufferCI.bTargetsSwapchain = info->bTargetsSwapchain;
			framebufferCI.bUsedByImGui = info->bImGUI && !info->bTargetsSwapchain ? true : false;
			framebufferCI.Attachments = { FramebufferAttachment(AttachmentFormat::Color) };

			Framebuffer::Create(framebufferCI, &m_Framebuffer);
		}

		// Creates editor camera
		if (m_UseEditorCamera)
		{
			m_EditorCamera = new EditorCamera(info->EditorCameraCI);
		}

		// Initialize ImGUI and renderers
		if (m_UseImGUI)
		{
			m_ImGuiContext.Init();
		}

		Renderer::Init();
		Renderer2D::Init();
#ifdef  FROSTIUM_OPENGL_IMPL
		GetOpenglRendererAPI()->Init();
#endif
		m_Initialized = true;
	}

	GraphicsContext::~GraphicsContext()
	{
		ShutDown();
		s_Instance = nullptr;
	}

	void GraphicsContext::SwapBuffers()
	{
		if (m_UseImGUI)
			m_ImGuiContext.OnEnd();
#ifdef  FROSTIUM_OPENGL_IMPL
		m_OpenglContext.SwapBuffers();
#else
		m_VulkanContext.SwapBuffers();
#endif
	}

	void GraphicsContext::ProcessEvents()
	{
		m_Window.ProcessEvents();
	}

	void GraphicsContext::BeginFrame(DeltaTime time)
	{
		if (m_UseEditorCamera)
			m_EditorCamera->OnUpdate(time);
#ifdef  FROSTIUM_OPENGL_IMPL
#else
		m_VulkanContext.BeginFrame();
#endif
		if (m_UseImGUI)
			m_ImGuiContext.OnBegin();
	}

	void GraphicsContext::ShutDown()
	{
		if (m_Initialized)
		{
			if(m_UseEditorCamera)
				delete m_EditorCamera;
			if (m_UseImGUI)
				m_ImGuiContext.ShutDown();

			Renderer::Shutdown();
			Renderer2D::Shutdown();
			m_Window.ShutDown();
#ifdef FROSTIUM_OPENGL_IMPL
#else
			m_VulkanContext.~VulkanContext();
#endif
			m_Initialized = false;
	}
}

	void GraphicsContext::OnResize(uint32_t height, uint32_t width)
	{
#ifdef  FROSTIUM_OPENGL_IMPL
		SetViewport(0, 0, width, height);
#else
		m_VulkanContext.OnResize(height, width);
#endif
		m_Framebuffer.OnResize(width, height);
	}

	void GraphicsContext::SetEventCallback(std::function<void(Event&)> callback)
	{
		m_EventCallback = callback;
	}

	DeltaTime GraphicsContext::CalculateDeltaTime()
	{
		float time = (float)glfwGetTime();
		DeltaTime deltaTime = time - m_LastFrameTime;
		m_LastFrameTime = time;
		return deltaTime;
	}

	Framebuffer* GraphicsContext::GetFramebuffer()
	{
		return &m_Framebuffer;
	}

	EditorCamera* GraphicsContext::GetEditorCamera()
	{
		return m_EditorCamera;
	}

	GLFWwindow* GraphicsContext::GetNativeWindow()
	{
		return m_Window.GetNativeWindow();
    }

	WindowData* GraphicsContext::GetWindowData()
	{
		return m_Window.GetWindowData();
	}

	float GraphicsContext::GetTime()
	{
		return (float)glfwGetTime();
	}

	float GraphicsContext::GetLastFrameTime()
	{
		return m_LastFrameTime;
	}

	void GraphicsContext::OnEvent(Event& event)
	{
		if (m_UseImGUI)
			m_ImGuiContext.OnEvent(event);

		if (m_UseEditorCamera)
			m_EditorCamera->OnEvent(event);

		if (Input::IsEventReceived(EventType::S_WINDOW_RESIZE, event))
		{
			auto& resize_event = static_cast<WindowResizeEvent&>(event);
			uint32_t width = resize_event.GetWidth();
			uint32_t height = resize_event.GetHeight();
#ifdef  FROSTIUM_OPENGL_IMPL
#else
			m_VulkanContext.OnResize(width, height);
#endif
			m_Framebuffer.OnResize(width, height);
		}

		m_EventCallback(std::forward<Event&>(event));
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

	GraphicsContext* GraphicsContext::GetSingleton()
	{
		return s_Instance;
	}
}