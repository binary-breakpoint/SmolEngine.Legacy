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

namespace Frostium 
{
	struct WindowCreateInfo;
	struct WindowData;
	class Framebuffer;

	struct GraphicsContextInitInfo
	{
		bool                bMSAA = true;
		bool                bTargetsSwapchain = true;
		uint32_t            Width = 1920;
		uint32_t            Height = 1080;

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

		/// Getters

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

		static GraphicsContext*              s_Instance;
		bool                                 m_Initialized = false;
#ifdef  FROSTIUM_OPENGL_IMPL
		OpenglContext                        m_OpenglContext = {};
		OpenglRendererAPI*                   m_RendererAPI = nullptr;
#else
		VulkanContext                        m_VulkanContext = {};
#endif
		Framebuffer                          m_Framebuffer = {};
		Window                               m_Window = {};

	private:

		friend class GraphicsPipeline;
		friend class Renderer;
		friend class Renderer2D;
		friend class Window;
	};
}
