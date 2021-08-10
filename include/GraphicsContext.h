#pragma once
#include "Camera/EditorCamera.h"

#ifdef  FROSTIUM_OPENGL_IMPL
#include "Backends/OpenGL/OpenglContext.h"
#include "Backends/OpenGL/OpenglRendererAPI.h"
#else
#include "Backends/Vulkan/VulkanContext.h"
#endif

#include "Tools/DefaultMeshes.h"
#include "Renderer/RendererShared.h"

#include "Common/Common.h"
#include "Common/Flags.h"
#include "Window/Window.h"

#include "Window/Events.h"

#include "Camera/Camera.h"
#include "Camera/Frustum.h"

#include "Primitives/Mesh.h"
#include "Primitives/Framebuffer.h"
#include "Primitives/Texture.h"

#include "Environment/CubeMap.h"

#include "GUI/ImGuiContext.h"

#include <functional>

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	struct WindowCreateInfo;
	struct WindowData;
	struct GraphicsContextState;
	struct Renderer2DStorage;
	struct RendererStorage;
	class CubeMap;
	class Framebuffer;
	class MaterialLibrary;
	class JobsSystemInstance;

	struct GraphicsContextInitInfo
	{
		bool                          bTargetsSwapchain = true;
		bool                          bAutoResize = true;
		bool                          bVsync= true;
		Flags                         Flags = Features_Renderer_3D_Flags | Features_Renderer_2D_Flags | Features_ImGui_Flags;
		MSAASamples                   eMSAASamples = MSAASamples::SAMPLE_COUNT_1;
		ShadowMapSize                 eShadowMapSize = ShadowMapSize::SIZE_8;
		Camera*                       pDefaultCamera = nullptr;
		WindowCreateInfo*             pWindowCI = nullptr;
		Renderer2DStorage*            pRenderer2DStorage = nullptr;
		RendererStorage*              pRendererStorage = nullptr;
		std::string                   ResourcesFolderPath = "../resources/";
	};

	class GraphicsContext
	{
	public:

		GraphicsContext() = default;
		GraphicsContext(GraphicsContextInitInfo* info);
		~GraphicsContext();
								      
		void                          ProcessEvents();
		void                          BeginFrame(float time);
		void                          UpdateSceneInfo(BeginSceneInfo* sceneInfo = nullptr);
		void                          SwapBuffers();
		void                          ShutDown();
								      
		// Getters				      
		static GraphicsContext*       GetSingleton();
		Camera*                       GetDefaultCamera() const;
		Framebuffer*                  GetFramebuffer() const;
		GLFWwindow*                   GetNativeWindow();
		Window*                       GetWindow() const;
	    WindowData*                   GetWindowData();
		Frustum*                      GetFrustum() const;
		DefaultMeshes*                GetDefaultMeshes() const;
		Texture*                      GetWhiteTexture() const;
		float                         GetGltfTime() const;
		float                         GetDeltaTime() const;
		float                         GetLastFrameTime() const;
		std::string                   GetResourcesPath() const;
#ifdef  FROSTIUM_OPENGL_IMPL	      
		static OpenglRendererAPI*     GetOpenglRendererAPI();
#else							      
		static VulkanContext&         GetVulkanContext();
#endif
		JobsSystemInstance*           GetJobsSystemInstance();
		// Setters				      
		void                          SetEventCallback(std::function<void(Event&)> callback);
		void                          SetDebugLogCallback(const std::function<void(const std::string&&, LogLevel)>& callback);
		void                          SetFramebufferSize(uint32_t width, uint32_t height);

		float                         CalculateDeltaTime();
		bool                          IsWindowMinimized() const;      
		void                          Resize(uint32_t* width, uint32_t* height);

	private:					      
		void                          OnEvent(Event& event);
		bool                          InitRenderer2DStorage(Renderer2DStorage* storage);
		bool                          InitRendererStorage(RendererStorage* storage, ShadowMapSize shadow_map_size);

	private:	
		static GraphicsContext*       s_Instance;
		Texture*                      m_DummyTexure = nullptr;
		CubeMap*                      m_DummyCubeMap = nullptr;
		Camera*                       m_DefaultCamera = nullptr;
		MaterialLibrary*              m_MaterialLibrary = nullptr;
		Renderer2DStorage*            m_Renderer2DStorage = nullptr;
		RendererStorage*              m_RendererStorage = nullptr;
		Frustum*                      m_Frustum = nullptr;
		Framebuffer*                  m_Framebuffer = nullptr;
		Window*                       m_Window = nullptr;
		DefaultMeshes*                m_DefaultMeshes = nullptr;
		ImGuiContext*                 m_ImGuiContext = nullptr;
		JobsSystemInstance*           m_JobsSystem = nullptr;
		bool                          m_bWindowMinimized = false;
		bool                          m_bIs2DStoragePreAlloc = false;
		bool                          m_bIsStoragePreAlloc = false;
		float                         m_LastFrameTime = 1.0f;
		float                         m_DeltaTime = 0.0f;
#ifdef  FROSTIUM_OPENGL_IMPL		  
		OpenglContext                 m_OpenglContext = {};
		OpenglRendererAPI*            m_RendererAPI = nullptr;
#else								  
		VulkanContext                 m_VulkanContext = {};
#endif
		GraphicsContextInitInfo       m_CreateInfo = {};
		EventSender                   m_EventHandler = {};
		SceneData                     m_SceneData = {};
		std::string                   m_ResourcesFolderPath = "";
		std::function<void(Event&)>   m_EventCallback;

	private:

		friend class GraphicsPipeline;
		friend class DeferredRenderer;
		friend class Renderer2D;
		friend class DebugRenderer;
		friend class ImGuiContext;
		friend class VulkanPBR;
		friend class VulkanContext;
		friend class VulkanDescriptor;
		friend class Window;
		friend class EnvironmentMap;
	};
}
