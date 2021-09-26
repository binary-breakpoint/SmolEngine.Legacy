#pragma once

#include "GUI/Backends/ContextBaseGUI.h"

namespace SmolEngine
{
	struct GraphicsContextInitInfo;

	class Window;
	class Framebuffer;
	class VulkanSwapchain;

	class ImGuiContext
	{
	public:
		static ContextBaseGUI* CreateContext();
	};

}
