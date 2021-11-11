#include "stdafx.h"
#ifdef OPENGL_IMPL
#else
#include "Backends/Vulkan/VulkanIndexBuffer.h"
#endif

namespace SmolEngine
{
	Ref<IndexBuffer> IndexBuffer::Create()
	{
		Ref<IndexBuffer> ib = nullptr;
#ifdef OPENGL_IMPL
#else
		ib = std::make_shared<VulkanIndexBuffer>();
#endif
		return ib;
	}

	uint32_t IndexBuffer::GetCount() const
	{
		return m_Elements;
	}
}