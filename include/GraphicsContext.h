#pragma once
#include "EditorCamera.h"

#ifdef  FROSTIUM_OPENGL_IMPL
#include "OpenGL/OpenglContext.h"
#include "OpenGL/OpenglRendererAPI.h"
#else
#include "Vulkan/VulkanContext.h"
#endif

#include "Common/Core.h"
#include "Common/Window.h"
#include "Common/Framebuffer.h"
#include "Common/Events.h"
#include "Common/Texture.h"

#include "ImGUI/ImGuiContext.h"

#include <functional>

namespace Frostium 
{
	struct WindowCreateInfo;
	struct WindowData;
	class Framebuffer;
	class MaterialLibrary;

	struct GraphicsContextInitInfo
	{
		bool                     bMSAA = true;
		bool                     bTargetsSwapchain = true;
		bool                     bImGUI = true;
		std::string              ResourcesFolderPath = "../resources/";

		EditorCameraCreateInfo*  pEditorCameraCI = nullptr;
		WindowCreateInfo*        pWindowCI = nullptr;
	};

	class GraphicsContext
	{
	public:

		GraphicsContext(GraphicsContextInitInfo* info);

		~GraphicsContext();


		void ProcessEvents();

		void BeginFrame(DeltaTime time);

		void SwapBuffers();

		void ShutDown();

		// Setters

		void SetEventCallback(std::function<void(Event&)> callback);

		// Helpers

		DeltaTime CalculateDeltaTime();

		// Getters

#ifdef  FROSTIUM_OPENGL_IMPL
		static OpenglRendererAPI* GetOpenglRendererAPI();
#else
		static VulkanContext& GetVulkanContext();
#endif
		static GraphicsContext* GetSingleton();

	    Framebuffer* GetFramebuffer();
	   
	    EditorCamera* GetEditorCamera();
	   
	    GLFWwindow* GetNativeWindow();
	   
	    WindowData* GetWindowData();

		bool IsWindowMinimized() const;
	   
	    float GetTime() const;

		float GetLastFrameTime() const;

	private:

		void OnResize(uint32_t* width, uint32_t* height);

		void OnEvent(Event& event);

	private:

		static GraphicsContext*              s_Instance;
		Ref<Texture>                         m_DummyTexure = nullptr;
		EditorCamera*                        m_EditorCamera = nullptr;
		MaterialLibrary*                     m_MaterialLibrary = nullptr;
		bool                                 m_Initialized = false;
		bool                                 m_WindowMinimized = false;
		const bool                           m_UseImGUI = false;
		const bool                           m_UseEditorCamera = false;
		float                                m_LastFrameTime = 1.0f;
#ifdef  FROSTIUM_OPENGL_IMPL
		OpenglContext                        m_OpenglContext = {};
		OpenglRendererAPI*                   m_RendererAPI = nullptr;
#else
		VulkanContext                        m_VulkanContext = {};
#endif
		Framebuffer                          m_Framebuffer = {};
		Window                               m_Window = {};
		ImGuiContext                         m_ImGuiContext = {};
		EventSender                          m_EventHandler = {};

		std::string                          m_ResourcesFolderPath = "";
		std::function<void(Event&)>          m_EventCallback;

	private:

		friend class GraphicsPipeline;
		friend class Renderer;
		friend class Renderer2D;
		friend class ImGuiContext;
		friend class VulkanPBR;
		friend class VulkanDescriptor;
		friend class Window;
	};
}
