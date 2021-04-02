#pragma once

#ifdef  FROSTIUM_OPENGL_IMPL
#include "OpenGL/OpenglContext.h"
#include "OpenGL/OpenglRendererAPI.h"
#else
#include "Vulkan/VulkanContext.h"
#endif

#include "Common/Core.h"
#include "Common/Window.h"
#include "Common/Framebuffer.h"
#include "Common/EventHandler.h"
#include "Common/Texture.h"

#include "ImGUI/ImGuiContext.h"

#include <functional>

namespace Frostium 
{
	struct WindowCreateInfo;
	struct WindowData;
	class Framebuffer;

	struct GraphicsContextInitInfo
	{
		bool                bMSAA = true;
		bool                bTargetsSwapchain = true;
		std::string         ResourcesFolderPath = "../resources/";

		WindowCreateInfo*   WindowInfo = nullptr;
	};

	class GraphicsContext
	{
	public:

	    static bool Init(GraphicsContextInitInfo* info);

		static void SwapBuffers();

		static void BeginFrame();

		static void ShutDown();

		static void OnResize(uint32_t width, uint32_t height);

		// Setters

		static void SetEventCallback(std::function<void(Event&)> callback);

		// Getters

#ifdef  FROSTIUM_OPENGL_IMPL
		static OpenglRendererAPI* GetOpenglRendererAPI();
#else
		static VulkanContext& GetVulkanContext();
#endif
		static Framebuffer* GetFramebuffer();

		static GLFWwindow* GetNativeWindow();

		static GraphicsContext* GetSingleton();

		static WindowData* GetWindowData();

	private:

		void OnEvent(Event& event);

	private:

		static GraphicsContext*              s_Instance;
		Ref<Texture>                         m_DummyTexure = nullptr;
		bool                                 m_Initialized = false;
#ifdef  FROSTIUM_OPENGL_IMPL
		OpenglContext                        m_OpenglContext = {};
		OpenglRendererAPI*                   m_RendererAPI = nullptr;
#else
		VulkanContext                        m_VulkanContext = {};
#endif
		Framebuffer                          m_Framebuffer = {};
		Window                               m_Window = {};
		ImGuiContext                         m_ImGuiContext = {};
		EventHandler                         m_EventHandler = {};

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
