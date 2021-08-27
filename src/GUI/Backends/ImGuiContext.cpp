#include "stdafx.h"
#include "GUI/Backends/ImGuiContext.h"

#ifdef FROSTIUM_OPENGL_IMPL
#else
#include "Backends/Vulkan/GUI/ImGuiVulkanImpl.h"
#endif

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	ContextBaseGUI* ImGuiContext::CreateContext()
	{
#ifdef FROSTIUM_OPENGL_IMPL
#else
		return new ImGuiVulkanImpl();
#endif
	}
}
