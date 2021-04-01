#include "stdafx.h"
#include "Common/IndexBuffer.h"

namespace Frostium
{
	void IndexBuffer::Bind() const
	{
#ifdef FROSTIUM_OPENGL_IMPL

		m_OpenglIndexBuffer.Bind();
#endif

	}

	void IndexBuffer::UnBind() const
	{
#ifdef FROSTIUM_OPENGL_IMPL

		m_OpenglIndexBuffer.UnBind();
#endif

	}

	void IndexBuffer::UploadData(uint32_t* indices, uint32_t count)
	{
#ifdef FROSTIUM_OPENGL_IMPL
		m_OpenglIndexBuffer.UploadData(indices, count);
#else
		m_VulkanIndexBuffer.SetData(indices, count);
#endif
	}

	void IndexBuffer::Destory()
	{
#ifdef FROSTIUM_OPENGL_IMPL

		m_OpenglIndexBuffer.Destroy();

#else
		m_VulkanIndexBuffer.Destroy();
#endif
	}

	uint32_t IndexBuffer::GetCount() const
	{
#ifdef FROSTIUM_OPENGL_IMPL

		return m_OpenglIndexBuffer.GetCount();
#else
		return m_VulkanIndexBuffer.GetCount();
#endif

	}

	Ref<IndexBuffer> IndexBuffer::Create(uint32_t* indices, uint32_t count)
	{
		Ref<IndexBuffer> buffer = std::make_shared<IndexBuffer>();

#ifdef FROSTIUM_OPENGL_IMPL

		buffer->m_OpenglIndexBuffer.Init(indices, count);
#else
		buffer->m_VulkanIndexBuffer.Create(indices, count);
#endif
		return buffer;
	}
}