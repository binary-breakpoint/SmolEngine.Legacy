#include "stdafx.h"
#include "GraphicsContext.h"
#include "DeferredRenderer.h"
#include "Renderer2D.h"
#include "DebugRenderer.h"
#include "MaterialLibrary.h"

#include "Common/SLog.h"
#include "Common/Input.h"
#include "Common/Common.h"

#include "Common/Renderer2DStorage.h"
#include "Common/RendererStorage.h"

#ifdef FROSTIUM_SMOLENGINE_IMPL
#include "Extensions/JobsSystemInstance.h"
#endif

#include <GLFW/glfw3.h>

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	GraphicsContext* GraphicsContext::s_Instance = nullptr;

	GraphicsContext::GraphicsContext(GraphicsContextInitInfo* info)
	{
		if (m_State != nullptr || !info->pWindowCI)
			return;

		m_State = new GraphicsContextState(info->pDefaultCamera != nullptr,
			info->Flags & Features_ImGui_Flags, info->bTargetsSwapchain, info->bAutoResize);

		s_Instance = this;
		m_Flags = info->Flags;
		m_MSAASamples = info->eMSAASamples;
		m_ResourcesFolderPath = info->ResourcesFolderPath;
		m_DefaultCamera = info->pDefaultCamera;
		m_EventHandler.OnEventFn = std::bind(&GraphicsContext::OnEvent, this, std::placeholders::_1);

		// Creates GLFW window
		m_Window = new Window();
		m_Window->Init(info->pWindowCI, &m_EventHandler);
		GLFWwindow* window = m_Window->GetNativeWindow();

		// Creates API context
#ifdef  FROSTIUM_OPENGL_IMPL
		m_OpenglContext.Setup(window);
#else
		m_VulkanContext.Setup(window, m_State,
			&m_Window->GetWindowData()->Width, &m_Window->GetWindowData()->Height);
#endif
		// Creates default frustum
		m_Frustum = new Frustum();

		// Creates 4x4 white texture
		m_DummyTexure = new Texture();
		Texture::CreateWhiteTexture(m_DummyTexure);

		// Initialize ImGUI
		if (m_State->UseImGUI)
		{
			m_ImGuiContext = new ImGuiContext();
			m_ImGuiContext->Init(info);
		}

#ifdef FROSTIUM_SMOLENGINE_IMPL
		m_JobsSystem = new JobsSystemInstance();
#endif
		LoadMeshes();
		// Creates main framebuffer
		m_Framebuffer = new Framebuffer();
		FramebufferSpecification framebufferCI = {};
		{
#ifdef  FROSTIUM_OPENGL_IMPL
			framebufferCI.Width = info->pWindowCI->Width;
			framebufferCI.Height = info->pWindowCI->Height;
#else
			framebufferCI.Width = m_VulkanContext.GetSwapchain().GetWidth();
			framebufferCI.Height = m_VulkanContext.GetSwapchain().GetHeight();
#endif
			framebufferCI.eMSAASampels = MSAASamples::SAMPLE_COUNT_1;
			framebufferCI.bTargetsSwapchain = info->bTargetsSwapchain;
			framebufferCI.bResizable = true;
			framebufferCI.bUsedByImGui = m_State->UseImGUI && !info->bTargetsSwapchain ? true : false;
			framebufferCI.Attachments = { FramebufferAttachment(AttachmentFormat::Color) };

			Framebuffer::Create(framebufferCI, m_Framebuffer);
		}

		if (info->Flags & Features_Renderer_3D_Flags)
		{
			if (info->pRendererStorage != nullptr)
			{
				m_RendererStorage = info->pRendererStorage;
				m_State->IsStoragePreAlloc = true;
			}
			else
			{
				m_RendererStorage = new RendererStorage();
			}

			InitRendererStorage(m_RendererStorage, info->eShadowMapSize);
			DeferredRenderer::Init(m_RendererStorage);

			// Adds default material
			MaterialCreateInfo materialInfo = {};
			materialInfo.SetMetalness(0.2f);
			materialInfo.SetRoughness(1.0f);
			m_MaterialLibrary = new MaterialLibrary();
			int32_t id = m_MaterialLibrary->Add(&materialInfo, "default material");
			DeferredRenderer::UpdateMaterials();
		}

		if (info->Flags & Features_Renderer_2D_Flags)
		{
			if (info->pRenderer2DStorage != nullptr)
			{
				m_Renderer2DStorage = info->pRenderer2DStorage;
				m_State->Is2DStoragePreAlloc = true;
			}
			else
			{
				m_Renderer2DStorage = new Renderer2DStorage();
			}

			InitRenderer2DStorage(m_Renderer2DStorage);
			Renderer2D::Init(m_Renderer2DStorage);
		}

		if (info->Flags & Features_Renderer_Debug_Flags)
		{
			DebugRenderer::Init();
		}

#ifdef  FROSTIUM_OPENGL_IMPL
		GetOpenglRendererAPI()->Init();
#endif
	}

	GraphicsContext::~GraphicsContext()
	{
		s_Instance = nullptr;
	}

	void GraphicsContext::SwapBuffers()
	{
		if (m_State->UseImGUI)
			m_ImGuiContext->OnEnd();
#ifdef  FROSTIUM_OPENGL_IMPL
		m_OpenglContext.SwapBuffers();
#else
		m_VulkanContext.SwapBuffers();
#endif
	}

	void GraphicsContext::ProcessEvents()
	{
		m_Window->ProcessEvents();
	}

	void GraphicsContext::BeginFrame(DeltaTime time)
	{
		m_DeltaTime = time;

		if (m_State->UseEditorCamera)
			m_DefaultCamera->OnUpdate(m_DeltaTime);
#ifdef  FROSTIUM_OPENGL_IMPL
#else
		m_VulkanContext.BeginFrame();
#endif
		if (m_State->UseImGUI)
			m_ImGuiContext->OnBegin();
	}

	void GraphicsContext::UpdateSceneInfo(BeginSceneInfo* info)
	{
		if (m_State->UseEditorCamera && info == nullptr)
		{
			Camera* camera = m_DefaultCamera;

			m_SceneData.View = camera->GetViewMatrix();
			m_SceneData.Projection = camera->GetProjection();
			m_SceneData.CamPos = glm::vec4(camera->GetPosition(), 1.0f);
			m_SceneData.SkyBoxMatrix = glm::mat4(glm::mat3(camera->GetViewMatrix()));
			m_SceneData.NearClip = camera->GetNearClip();
			m_SceneData.FarClip = camera->GetFarClip();
		}

		if (info)
		{
			m_SceneData.View = info->View;
			m_SceneData.Projection = info->Proj;
			m_SceneData.CamPos = glm::vec4(info->Pos, 1);
			m_SceneData.SkyBoxMatrix = glm::mat4(glm::mat3(info->View));
			m_SceneData.NearClip = info->NearClip;
			m_SceneData.FarClip = info->FarClip;
		}

		m_Frustum->Update(m_SceneData.Projection * m_SceneData.View);
	}

	void GraphicsContext::ShutDown()
	{
		if (m_State != nullptr)
		{
			if (m_State->UseImGUI)
				m_ImGuiContext->ShutDown();

			DeferredRenderer::Shutdown();
			Renderer2D::Shutdown();

			m_Window->ShutDown();
#ifdef FROSTIUM_OPENGL_IMPL
#else
			m_VulkanContext.~VulkanContext();
#endif
			if (!m_State->Is2DStoragePreAlloc)
				delete m_Renderer2DStorage;

			if (!m_State->IsStoragePreAlloc)
				delete m_RendererStorage;

			delete m_MaterialLibrary, m_DummyTexure, m_State,
				m_Framebuffer, m_Window, m_ImGuiContext,
				m_BoxMesh, m_SphereMesh, m_CapsuleMesh;

#ifdef FROSTIUM_SMOLENGINE_IMPL
			delete m_JobsSystem;
#endif
	    }
}

	void GraphicsContext::Resize(uint32_t* width, uint32_t* height)
	{
#ifdef  FROSTIUM_OPENGL_IMPL
#else
		m_VulkanContext.OnResize(width, height);
#endif
		if (m_State->AutoResize)
			SetFramebufferSize(*width, *height);
	}

	void GraphicsContext::SetEventCallback(std::function<void(Event&)> callback)
	{
		m_EventCallback = callback;
	}

	void GraphicsContext::SetFramebufferSize(uint32_t width, uint32_t height)
	{
		m_Framebuffer->OnResize(width, height);
		if (m_Flags & Features_Renderer_3D_Flags)
			DeferredRenderer::OnResize(width, height);
	}

	DeltaTime GraphicsContext::CalculateDeltaTime()
	{
		float time = (float)glfwGetTime();
		DeltaTime deltaTime = time - m_LastFrameTime;
		m_LastFrameTime = time;
		return deltaTime;
	}

	Framebuffer* GraphicsContext::GetFramebuffer() const
	{
		return m_Framebuffer;
	}

	Camera* GraphicsContext::GetDefaultCamera() const
	{
		return m_DefaultCamera;
	}

	GLFWwindow* GraphicsContext::GetNativeWindow()
	{
		return m_Window->GetNativeWindow();
    }

	WindowData* GraphicsContext::GetWindowData()
	{
		return m_Window->GetWindowData();
	}

	Frustum* GraphicsContext::GetFrustum() const
	{
		return m_Frustum;
	}

	Mesh* GraphicsContext::GetBoxMesh() const
	{
		return m_BoxMesh;
	}

	Mesh* GraphicsContext::GetCapsuleMesh() const
	{
		return m_CapsuleMesh;
	}

	Mesh* GraphicsContext::GetSphereMesh() const
	{
		return m_SphereMesh;
	}

	Texture* GraphicsContext::GetWhiteTexture() const
	{
		return m_DummyTexure;
	}

	bool GraphicsContext::IsWindowMinimized() const
	{
		return m_State->WindowMinimized;
	}

	bool GraphicsContext::InitRenderer2DStorage(Renderer2DStorage* storage)
	{
		if (!storage) {
			return false;
		}
		storage->Frustum = m_Frustum;
		return true;
	}

	bool GraphicsContext::InitRendererStorage(RendererStorage* storage, ShadowMapSize shadow_map_size)
	{
		if (!storage) {
			return false;
		}
		storage->m_MapSize = shadow_map_size;
		storage->m_Path = m_ResourcesFolderPath;
		storage->m_Frustum = m_Frustum;
		return true;
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

	void GraphicsContext::OnEvent(Event& e)
	{
		if (m_State->UseImGUI)
			m_ImGuiContext->OnEvent(e);

		if (m_State->UseEditorCamera)
			m_DefaultCamera->OnEvent(e);

		if (e.IsType(EventType::WINDOW_RESIZE))
		{
			WindowResizeEvent* resize = e.Cast<WindowResizeEvent>();
			uint32_t width = resize->GetWidth();
			uint32_t height = resize->GetHeight();

			if (width == 0 || height == 0)
				m_State->WindowMinimized = true;
			else
			{
				m_State->WindowMinimized = false;
				Resize(&width, &height);
			}
		}

		if(m_EventCallback != nullptr)
			m_EventCallback(std::forward<Event>(e));
	}

	bool GraphicsContext::LoadMeshes()
	{
		m_BoxMesh = new Mesh();
		m_CapsuleMesh = new Mesh();
		m_SphereMesh = new Mesh();

		Mesh::Create(m_ResourcesFolderPath + "Models/box.gltf", m_BoxMesh);
		Mesh::Create(m_ResourcesFolderPath + "Models/capsule.gltf", m_CapsuleMesh);
		Mesh::Create(m_ResourcesFolderPath + "Models/sphere.gltf", m_SphereMesh);
		return true;
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

#ifdef FROSTIUM_SMOLENGINE_IMPL
	JobsSystemInstance* GraphicsContext::GetJobsSystemInstance()
	{
		return m_JobsSystem;
	}
#endif
}