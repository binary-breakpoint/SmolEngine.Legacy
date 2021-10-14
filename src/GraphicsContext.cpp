#include "stdafx.h"
#include "GraphicsContext.h"
#include "Renderer/RendererDebug.h"
#include "Renderer/RendererShared.h"
#include "Common/DebugLog.h"
#include "Window/Input.h"

#ifdef SMOLENGINE_OPENGL_IMPL
#else
#include "Backends/Vulkan/VulkanContext.h"
#endif

#include <GLFW/glfw3.h>

namespace SmolEngine
{
	GraphicsContext* GraphicsContext::s_Instance = nullptr;

	GraphicsContext::GraphicsContext()
	{
		s_Instance = this;
	}

	GraphicsContext::~GraphicsContext()
	{
		s_Instance = nullptr;

		Shutdown();
	}

	void GraphicsContext::Initialize(GraphicsContextCreateInfo* info)
	{
		m_CreateInfo = *info;
		m_Root = info->ResourcesFolder;
		m_EventHandler.OnEventFn = std::bind(&GraphicsContext::OnEvent, this, std::placeholders::_1);

		// Creates GLFW window
		m_Window = std::make_shared<Window>();
		m_Window->Init(info->pWindowCI, &m_EventHandler);
		GLFWwindow* window = m_Window->GetNativeWindow();

		// Creates API context
		CreateAPIContextEX();

		m_JobsSystem = std::make_shared< JobsSystemInstance>();
		m_MeshPool = std::make_shared<MeshPool>(info->ResourcesFolder);
		m_MaterialPool = std::make_shared<MaterialPool>();
		m_TexturePool = std::make_shared<TexturePool>();

		// Creates default framebuffer
		m_Framebuffer = Framebuffer::Create();
		FramebufferSpecification framebufferCI = {};
		framebufferCI.Width = m_CreateInfo.pWindowCI->Width;
		framebufferCI.Height = m_CreateInfo.pWindowCI->Height;
		framebufferCI.eMSAASampels = MSAASamples::SAMPLE_COUNT_1;
		framebufferCI.bTargetsSwapchain = info->bTargetsSwapchain;
		framebufferCI.bResizable = true;
		framebufferCI.bAutoSync = false;
		framebufferCI.bUsedByImGui = (m_CreateInfo.eFeaturesFlags & FeaturesFlags::Imgui) == FeaturesFlags::Imgui && !info->bTargetsSwapchain ? true : false;
		framebufferCI.Attachments = { FramebufferAttachment(AttachmentFormat::Color) };
		m_Framebuffer->Build(&framebufferCI);

		// Initialize debug renderer
		if ((info->eFeaturesFlags & FeaturesFlags::RendererDebug) == FeaturesFlags::RendererDebug)
		{
			RendererDebug::Init();
		}

		OnContexReadyEX();
	}

	Ref<GraphicsContext> GraphicsContext::Create(GraphicsContextCreateInfo* info)
	{
		Ref<GraphicsContext> context = nullptr;
#ifdef SMOLENGINE_OPENGL_IMPL
#else
		context = std::make_shared<VulkanContext>();
#endif
		context->Initialize(info);
		return context;
	}

	void GraphicsContext::SwapBuffers()
	{
		SwapBuffersEX();
	}

	void GraphicsContext::ProcessEvents()
	{
		m_Window->ProcessEvents();
	}

	void GraphicsContext::BeginFrame(float time)
	{
		m_DeltaTime = time;
		BeginFrameEX(time);
	}

	void GraphicsContext::Shutdown()
	{
		m_Window->ShutDown();
    }

	void GraphicsContext::Resize(uint32_t* width, uint32_t* height)
	{
		ResizeEX(width, height);

		if (m_CreateInfo.bAutoResize)
		{
			SetFramebufferSize(*width, *height);
		}
	}

	void GraphicsContext::SetEventCallback(std::function<void(Event&)> callback)
	{
		m_EventCallback = callback;
	}

	void GraphicsContext::SetFramebufferSize(uint32_t width, uint32_t height)
	{
		m_Framebuffer->OnResize(width, height);
		for (auto storage : m_StorageList)
			storage->OnResize(width, height);
	}

	float GraphicsContext::CalculateDeltaTime()
	{
		float time = (float)glfwGetTime();
		float deltaTime = time - m_LastFrameTime;
		m_LastFrameTime = time;
		return deltaTime;
	}

	bool GraphicsContext::IsWindowMinimized() const
	{
		return m_bWindowMinimized;
	}

	Window* GraphicsContext::GetWindow() const
	{
		return m_Window.get();
	}

	float GraphicsContext::GetGltfTime() const
	{
		return (float)glfwGetTime();
	}

	float GraphicsContext::GetDeltaTime() const
	{
		return m_DeltaTime;
	}

	const std::string& GraphicsContext::GetResourcesPath() const
	{
		return m_Root;
	}

	void GraphicsContext::OnEvent(Event& e)
	{
		OnEventEX(e);

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

	GraphicsContext* GraphicsContext::GetSingleton()
	{
		return s_Instance;
	}

	Ref<Framebuffer> GraphicsContext::GetMainFramebuffer() const
	{
		return m_Framebuffer;
	}
}