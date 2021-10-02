#include "stdafx.h"
#include "Primitives/VertexBuffer.h"

#ifdef OPENGL_IMPL
#include "Backends/OpenGL/OpenglBuffer.h"
#else
#include "Backends/Vulkan/VulkanVertexBuffer.h"
#endif

namespace SmolEngine
{
	Ref<VertexBuffer> VertexBuffer::Create()
	{
		Ref<VertexBuffer> vb = nullptr;
#ifdef OPENGL_IMPL
#else
		vb = std::make_shared<VulkanVertexBuffer>();
#endif
		return vb;
	}
}