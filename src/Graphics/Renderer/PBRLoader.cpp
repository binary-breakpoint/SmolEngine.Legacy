#include "stdafx.h"
#include "Graphics/Renderer/PBRLoader.h"

#ifdef OPENGL_IMPL
#else
#include "Graphics/Backends/Vulkan/VulkanPBRLoader.h"
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