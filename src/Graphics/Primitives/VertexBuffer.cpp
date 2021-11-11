#include "stdafx.h"
#include "Graphics/Primitives/VertexBuffer.h"

#ifdef OPENGL_IMPL
#include "Graphics/Backends/OpenGL/OpenglBuffer.h"
#else
#include "Graphics/Backends/Vulkan/VulkanVertexBuffer.h"
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