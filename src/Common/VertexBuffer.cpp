#include "stdafx.h"
#include "Common/VertexBuffer.h"

namespace Frostium
{
	void VertexBuffer::SetLayout(const BufferLayout& layout)
	{
#ifdef FROSTIUM_OPENGL_IMPL
		m_OpenglVertexBuffer.SetLayout(layout);
#else
#endif
	}

	Ref<VertexBuffer> VertexBuffer::Create(uint32_t size)
	{
		Ref<VertexBuffer> buffer = std::make_shared<VertexBuffer>();
#ifdef FROSTIUM_OPENGL_IMPL
		buffer->m_OpenglVertexBuffer.Init(size);
#else
		buffer->m_VulkanVertexBuffer.Create(size);
#endif
		return buffer;
	}


	Ref<VertexBuffer> VertexBuffer::Create(void* vertices, uint32_t size)
	{
		Ref<VertexBuffer> buffer = std::make_shared<VertexBuffer>();
#ifdef FROSTIUM_OPENGL_IMPL
		buffer->m_OpenglVertexBuffer.Init(vertices, size);
#else
		buffer->m_VulkanVertexBuffer.Create(vertices, size);
#endif
		return buffer;
	}

	void VertexBuffer::Bind() const
	{
#ifdef FROSTIUM_OPENGL_IMPL
		m_OpenglVertexBuffer.Bind();
#endif
	}

	void VertexBuffer::UnBind() const
	{
#ifdef FROSTIUM_OPENGL_IMPL
		m_OpenglVertexBuffer.UnBind();
#endif
	}

	void VertexBuffer::Destory()
	{
#ifdef FROSTIUM_OPENGL_IMPL
		m_OpenglVertexBuffer.Destroy();
#else
		m_VulkanVertexBuffer.Destroy();
#endif
	}

	void VertexBuffer::UploadData(const void* data, const uint32_t size, const uint32_t offset)
	{
#ifdef FROSTIUM_OPENGL_IMPL
		m_OpenglVertexBuffer.UploadData(data, size, offset);
#else
		m_VulkanVertexBuffer.SetData(data, size, offset);
#endif
	}

	const BufferLayout& VertexBuffer::GetLayout() const
	{
#ifdef FROSTIUM_OPENGL_IMPL
		return m_OpenglVertexBuffer.GetLayout();
#else
		BufferLayout dummy;
		return dummy;
#endif
	}
}