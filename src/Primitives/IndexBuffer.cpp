#include "stdafx.h"
#include "Primitives/IndexBuffer.h"

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
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

	void IndexBuffer::Clear()
	{
#ifdef FROSTIUM_OPENGL_IMPL
#else
		m_VulkanIndexBuffer.Destroy();
#endif
	}

	bool IndexBuffer::IsInitialized() const
	{
#ifdef FROSTIUM_OPENGL_IMPL
		return false;
#else
		return m_VulkanIndexBuffer.GetSize() > 0;
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

	uint32_t IndexBuffer::GetCount() const
	{
#ifdef FROSTIUM_OPENGL_IMPL

		return m_OpenglIndexBuffer.GetCount();
#else
		return m_VulkanIndexBuffer.GetCount();
#endif

	}

	void IndexBuffer::Create(IndexBuffer* out_ib, uint32_t* indices, uint32_t count, bool is_static)
	{
		if (out_ib)
		{
#ifdef FROSTIUM_OPENGL_IMPL

			out_ib->m_OpenglIndexBuffer.Init(indices, count);
#else
			out_ib->m_VulkanIndexBuffer.Create(indices, count, is_static);
#endif
		}
	}
}