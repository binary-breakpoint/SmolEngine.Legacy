#include "stdafx.h"
#include "GraphicsContext.h"
#include "Renderer.h"
#include "Renderer2D.h"
#include "MaterialLibrary.h"

#include "Common/SLog.h"
#include "Common/Input.h"

#include <GLFW/glfw3.h>

namespace Frostium
{
	GraphicsContext* GraphicsContext::s_Instance = nullptr;

	GraphicsContext::GraphicsContext(GraphicsContextInitInfo* info)
		:
		m_UseImGUI(info->Flags & Features_ImGui_Flags),
		m_UseEditorCamera(info->pEditorCameraCI != nullptr ? true: false)
	{
		if (m_Initialized || !info->pWindowCI)
			return;

		s_Instance = this;
		// Initialize spdlog
		SLog::InitLog();
		m_ResourcesFolderPath = info->ResourcesFolderPath;
		m_EventHandler.OnEventFn = std::bind(&GraphicsContext::OnEvent, this, std::placeholders::_1);
		// Creates GLFW window
		m_Window.Init(info->pWindowCI, &m_EventHandler);
		GLFWwindow* window = m_Window.GetNativeWindow();
		// Creates API context
#ifdef  FROSTIUM_OPENGL_IMPL
		m_OpenglContext.Setup(window);
#else
		m_VulkanContext.Setup(window,
			&m_Window.GetWindowData()->Width, &m_Window.GetWindowData()->Height);
		m_VulkanContext.m_UseImGUI = m_UseImGUI;
#endif
		// Creates 4x4 white texture
		m_DummyTexure = new Texture();
		Texture::CreateWhiteTexture(m_DummyTexure);
		// Creates main framebuffer
		FramebufferSpecification framebufferCI = {};
		{
#ifdef  FROSTIUM_OPENGL_IMPL
			framebufferCI.Width = info->pWindowCI->Width;
			framebufferCI.Height = info->pWindowCI->Height;
#else
			framebufferCI.Width = m_VulkanContext.GetSwapchain().GetWidth();
			framebufferCI.Height = m_VulkanContext.GetSwapchain().GetHeight();
#endif
			framebufferCI.eMSAASampels = info->eMSAASamples;
			framebufferCI.bTargetsSwapchain = info->bTargetsSwapchain;
			framebufferCI.bResizable = true;
			framebufferCI.bUsedByImGui = m_UseImGUI && !info->bTargetsSwapchain ? true : false;
			framebufferCI.Attachments = { FramebufferAttachment(AttachmentFormat::Color) };

			Framebuffer::Create(framebufferCI, &m_Framebuffer);
		}

		// Creates editor camera
		if (m_UseEditorCamera)
		{
			m_EditorCamera = new EditorCamera(info->pEditorCameraCI);
		}

		// Initialize ImGUI and renderers
		if (m_UseImGUI)
		{
			m_ImGuiContext.Init();
		}

		if (info->Flags & Features_Renderer_3D_Flags)
		{
			Renderer::Init();

			// Adds default material
			MaterialCreateInfo materialInfo = {};
			materialInfo.SetMetalness(0.2f);
			materialInfo.SetRoughness(1.0f);
			m_MaterialLibrary = new MaterialLibrary();
			int32_t id = m_MaterialLibrary->Add(&materialInfo, "default material");
			Renderer::UpdateMaterials();
		}

		if (info->Flags & Features_Renderer_2D_Flags)
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
			delete m_MaterialLibrary, m_DummyTexure;
			if(m_UseEditorCamera)
				delete m_EditorCamera;

			if (m_UseImGUI)
				m_ImGuiContext.ShutDown();

			Renderer::Shutdown();
			Renderer2D::Shutdown();
			m_Framebuffer.~Framebuffer();
			m_Window.ShutDown();
#ifdef FROSTIUM_OPENGL_IMPL
#else
			m_VulkanContext.~VulkanContext();
#endif
			m_Initialized = false;
	    }
}

	void GraphicsContext::OnResize(uint32_t* width, uint32_t* height)
	{
#ifdef  FROSTIUM_OPENGL_IMPL
#else
		m_VulkanContext.OnResize(width, height);
#endif
		m_Framebuffer.OnResize(*width, *height);
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

	Frustum* GraphicsContext::GetFrustum()
	{
		return &m_Frustum;
	}

	bool GraphicsContext::IsWindowMinimized() const
	{
		return m_WindowMinimized;
	}

	Window* GraphicsContext::GetWindow()
	{
		return &m_Window;
	}

	float GraphicsContext::GetTime() const
	{
		return (float)glfwGetTime();
	}

	float GraphicsContext::GetLastFrameTime() const
	{
		return m_LastFrameTime;
	}

	void GraphicsContext::OnEvent(Event& event)
	{
		if (m_UseImGUI)
			m_ImGuiContext.OnEvent(event);

		if (m_UseEditorCamera)
			m_EditorCamera->OnEvent(event);

		if (event.IsType(EventType::WINDOW_RESIZE))
		{
			WindowResizeEvent* resize = event.Cast<WindowResizeEvent>();
			uint32_t width = resize->GetWidth();
			uint32_t height = resize->GetHeight();

			if (width == 0 || height == 0)
				m_WindowMinimized = true;
			else
			{
				m_WindowMinimized = false;
				OnResize(&width, &height);
			}
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