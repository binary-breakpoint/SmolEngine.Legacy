#include "stdafx.h"
#include "Common/VertexBuffer.h"

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	void VertexBuffer::SetLayout(const BufferLayout& layout)
	{
#ifdef FROSTIUM_OPENGL_IMPL
		m_OpenglVertexBuffer.SetLayout(layout);
#else
#endif
	}

	void VertexBuffer::Create(VertexBuffer* buffer, uint32_t size)
	{
		if (buffer)
		{
#ifdef FROSTIUM_OPENGL_IMPL
			buffer->m_OpenglVertexBuffer.Init(size);
#else
			buffer->m_VulkanVertexBuffer.Create(size);
#endif
		}
	}

	void VertexBuffer::Create(VertexBuffer* out_vb, void* vertices, uint32_t size, bool is_static)
	{
		if (out_vb)
		{
#ifdef FROSTIUM_OPENGL_IMPL
			out_vb->m_OpenglVertexBuffer.Init(vertices, size);
#else
			out_vb->m_VulkanVertexBuffer.Create(vertices, size, is_static);
#endif
		}
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

	void VertexBuffer::Clear()
	{
		if (IsInitialized())
		{
#ifdef FROSTIUM_OPENGL_IMPL
#else
			m_VulkanVertexBuffer.Destroy();
#endif
		}
	}

	bool VertexBuffer::IsInitialized() const
	{
#ifdef FROSTIUM_OPENGL_IMPL
		return false;
#else
		return m_VulkanVertexBuffer.GetSize() > 0;
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

	BufferLayout* VertexBuffer::GetLayout() 
	{
#ifdef FROSTIUM_OPENGL_IMPL
		return &m_OpenglVertexBuffer.GetLayout();
#else
		return nullptr;
#endif
	}
}