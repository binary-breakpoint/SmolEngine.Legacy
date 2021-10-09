#include "stdafx.h"
#include "GraphicsContext.h"

#include "Renderer/RendererDebug.h"
#include "Common/DebugLog.h"
#include "Common/Common.h"
#include "Window/Input.h"

#include "Multithreading/JobsSystemInstance.h"

#include <GLFW/glfw3.h>

namespace SmolEngine
{
	static int result = 0;
	GraphicsContext* GraphicsContext::s_Instance = nullptr;

	GraphicsContext::GraphicsContext(GraphicsContextInitInfo* info)
	{
		s_Instance = this;
		m_CreateInfo = *info;
		m_ResourcesFolderPath = info->ResourcesFolderPath;
		m_EventHandler.OnEventFn = std::bind(&GraphicsContext::OnEvent, this, std::placeholders::_1);

		// Creates GLFW window
		m_Window = new Window();
		m_Window->Init(info->pWindowCI, &m_EventHandler);
		GLFWwindow* window = m_Window->GetNativeWindow();
		// Creates API context
#ifdef  OPENGL_IMPL
		m_OpenglContext.Setup(window);
#else
		m_VulkanContext.Setup(window, this,
			&m_Window->GetWindowData()->Width, &m_Window->GetWindowData()->Height);
#endif
		// Creates 4x4 white textures
		{
			TextureCreateInfo texCI;
			texCI.Width = 4;
			texCI.Height = 4;

			m_DummyTexure = Texture::Create();
			m_DummyTexure->LoadAsWhite();

			m_StorageTexure = Texture::Create();
			m_StorageTexure->LoadAsStorage(&texCI);

			m_DummyCubeMap = Texture::Create();
			m_DummyCubeMap->LoadAsWhiteCube(&texCI);
		}

		m_JobsSystem = std::make_shared< JobsSystemInstance>();
		m_MeshPool = std::make_shared<MeshPool>();
		m_MeshPool->LoadDefaultMeshes(info->ResourcesFolderPath);
		m_MaterialPool = std::make_shared<MaterialPool>();

		// Initialize ImGUI
		if ((info->eFeaturesFlags & FeaturesFlags::Imgui) == FeaturesFlags::Imgui)
		{
			m_ImGuiContext = ImGuiContext::CreateContext();;
			m_ImGuiContext->Init();
		}

		// Creates default framebuffer
		m_Framebuffer = Framebuffer::Create();
		FramebufferSpecification framebufferCI = {};
#ifdef  OPENGL_IMPL
		framebufferCI.Width = info->pWindowCI->Width;
		framebufferCI.Height = info->pWindowCI->Height;
#else
		framebufferCI.Width = m_VulkanContext.GetSwapchain().GetWidth();
		framebufferCI.Height = m_VulkanContext.GetSwapchain().GetHeight();
#endif
		framebufferCI.eMSAASampels = MSAASamples::SAMPLE_COUNT_1;
		framebufferCI.bTargetsSwapchain = info->bTargetsSwapchain;
		framebufferCI.bResizable = true;
		framebufferCI.bAutoSync = false;
		framebufferCI.bUsedByImGui = (m_CreateInfo.eFeaturesFlags & FeaturesFlags::Imgui) == FeaturesFlags::Imgui && !info->bTargetsSwapchain ? true : false;
		framebufferCI.Attachments = { FramebufferAttachment(AttachmentFormat::Color) };

		m_Framebuffer->Build(&framebufferCI);

#ifdef  OPENGL_IMPL
		GetOpenglRendererAPI()->Init();
#endif
		// Initialize debug renderer
		if ((info->eFeaturesFlags & FeaturesFlags::RendererDebug) == FeaturesFlags::RendererDebug)
		{
			RendererDebug::Init();
		}

		m_NuklearContext = NuklearContext::CreateContext();
		m_NuklearContext->Init();
	}

	GraphicsContext::~GraphicsContext()
	{
		s_Instance = nullptr;
	}

	void GraphicsContext::SwapBuffers()
	{
		if ((m_CreateInfo.eFeaturesFlags & FeaturesFlags::Imgui) == FeaturesFlags::Imgui)
			m_ImGuiContext->EndFrame();

#ifdef  OPENGL_IMPL
		m_OpenglContext.SwapBuffers();
#else
		m_VulkanContext.SwapBuffers();
#endif
	}

	void GraphicsContext::ProcessEvents()
	{
		m_Window->ProcessEvents();
	}

	void GraphicsContext::BeginFrame(float time)
	{
		m_DeltaTime = time;
#ifdef  OPENGL_IMPL
#else
		m_VulkanContext.BeginFrame();
#endif
		if ((m_CreateInfo.eFeaturesFlags & FeaturesFlags::Imgui) == FeaturesFlags::Imgui)
			m_ImGuiContext->NewFrame();

		m_NuklearContext->NewFrame();

	}

	void GraphicsContext::ShutDown()
	{
		if ((m_CreateInfo.eFeaturesFlags & FeaturesFlags::Imgui) == FeaturesFlags::Imgui)
			m_ImGuiContext->ShutDown();

		m_Window->ShutDown();
#ifdef OPENGL_IMPL
#else
		m_VulkanContext.~VulkanContext();
#endif
		delete  m_Window, m_ImGuiContext, m_NuklearContext;
}

	void GraphicsContext::Resize(uint32_t* width, uint32_t* height)
	{
#ifdef  OPENGL_IMPL
#else
		m_VulkanContext.OnResize(width, height);
#endif
		if (m_CreateInfo.bAutoResize)
			SetFramebufferSize(*width, *height);
	}

	void GraphicsContext::SetEventCallback(std::function<void(Event&)> callback)
	{
		m_EventCallback = callback;
	}

	void GraphicsContext::SetFramebufferSize(uint32_t width, uint32_t height)
	{
		m_Framebuffer->OnResize(width, height);

		for (auto& storage : m_Storages)
			storage->OnResize(width, height);
	}

	float GraphicsContext::CalculateDeltaTime()
	{
		float time = (float)glfwGetTime();
		float deltaTime = time - m_LastFrameTime;
		m_LastFrameTime = time;
		return deltaTime;
	}

	GLFWwindow* GraphicsContext::GetNativeWindow()
	{
		return m_Window->GetNativeWindow();
    }

	WindowData* GraphicsContext::GetWindowData()
	{
		return m_Window->GetWindowData();
	}

	Ref<Texture> GraphicsContext::GetWhiteTexture() const
	{
		return m_DummyTexure;
	}

	bool GraphicsContext::IsWindowMinimized() const
	{
		return m_bWindowMinimized;
	}

	bool GraphicsContext::PushStorage(RendererStorageBase* storage)
	{
		bool found = std::find(m_Storages.begin(), m_Storages.end(), storage) != m_Storages.end();
		if (!found)
		{
			m_Storages.emplace_back(storage);
			return true;
		}

		return false;
	}

	bool GraphicsContext::PopStorage(RendererStorageBase* storage)
	{
		auto it = std::find(m_Storages.begin(), m_Storages.end(), storage);
		if (it != m_Storages.end())
		{
			m_Storages.erase(it);
			return true;
		}

		return false;
	}

	Window* GraphicsContext::GetWindow() const
	{
		return m_Window;
	}

	float GraphicsContext::GetGltfTime() const
	{
		return (float)glfwGetTime();
	}

	float GraphicsContext::GetDeltaTime() const
	{
		return m_DeltaTime;
	}

	float GraphicsContext::GetLastFrameTime() const
	{
		return m_LastFrameTime;
	}

	const std::string& GraphicsContext::GetResourcesPath() const
	{
		return m_ResourcesFolderPath;
	}

	void GraphicsContext::OnEvent(Event& e)
	{
		if ((m_CreateInfo.eFeaturesFlags & FeaturesFlags::Imgui) == FeaturesFlags::Imgui)
			m_ImGuiContext->OnEvent(e);

		m_NuklearContext->OnEvent(e);

		if (e.IsType(EventType::WINDOW_RESIZE))
		{
			WindowResizeEvent* resize = e.Cast<WindowResizeEvent>();
			uint32_t width = resize->GetWidth();
			uint32_t height = resize->GetHeight();

			if (width == 0 || height == 0)
				m_bWindowMinimized = true;
			else
			{
				m_bWindowMinimized = false;
				Resize(&width, &height);
			}
		}

		if(m_EventCallback != nullptr)
			m_EventCallback(std::forward<Event>(e));
	}

#ifdef OPENGL_IMPL
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

	Ref<Framebuffer> GraphicsContext::GetMainFramebuffer() const
	{
		return m_Framebuffer;
	}

#ifdef FROSTIUM_SMOLENGINE_IMPL
	JobsSystemInstance* GraphicsContext::GetJobsSystemInstance()
	{
		return m_JobsSystem;
	}
#endif
}