#include "stdafx.h"
#include "Renderer/PBRLoader.h"

#ifdef OPENGL_IMPL
#else
#include "Backends/Vulkan/VulkanPBRLoader.h"
#endif

namespace SmolEngine
{
	Ref<PBRLoader> PBRLoader::Create()
	{
#ifdef OPENGL_IMPL
#else
		return std::make_shared<VulkanPBRLoader>();
#endif
	}
}