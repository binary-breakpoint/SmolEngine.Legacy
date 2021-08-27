#pragma once

#include "GUI/Backends/ContextBaseGUI.h"

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
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
