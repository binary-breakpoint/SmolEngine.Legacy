#include "stdafx.h"
#include "GUI/Backends/ImGuiContext.h"

#ifdef OPENGL_IMPL
#else
#include "Backends/Vulkan/GUI/ImGuiVulkanImpl.h"
#endif

namespace SmolEngine
{
	ContextBaseGUI* ImGuiContext::CreateContext()
	{
#ifdef OPENGL_IMPL
#else
		return new ImGuiVulkanImpl();
#endif
	}
}
