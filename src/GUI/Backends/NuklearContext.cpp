#include "stdafx.h"
#include "GUI/Backends/NuklearContext.h"

#ifdef FROSTIUM_OPENGL_IMPL
#else
#include "Backends/Vulkan/GUI/NuklearVulkanImpl.h"
#endif

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	ContextBaseGUI* NuklearContext::CreateContext()
	{
#ifdef FROSTIUM_OPENGL_IMPL
#else
		return new NuklearVulkanImpl();
#endif
	}
}
