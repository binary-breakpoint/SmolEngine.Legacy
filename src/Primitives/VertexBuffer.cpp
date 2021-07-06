#include "stdafx.h"
#include "Primitives/VertexBuffer.h"

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	void VertexBuffer::Create(VertexBuffer* buffer, uint32_t size)
	{
		buffer->Init(size);
	}

	void VertexBuffer::Create(VertexBuffer* out_vb, void* vertices, uint32_t size, bool is_static)
	{
		if (out_vb)
		{
#ifdef FROSTIUM_OPENGL_IMPL
			out_vb->Init(vertices, size);
#else
			out_vb->Init(vertices, size, is_static);
#endif
		}
	}

	void VertexBuffer::Bind() const
	{
	}

	void VertexBuffer::UnBind() const
	{

	}

	void VertexBuffer::Clear()
	{
		if (IsReady())
		{
#ifdef FROSTIUM_OPENGL_IMPL
#else
			Destroy();
#endif
		}
	}

	bool VertexBuffer::IsReady() const
	{
		return GetSize() > 0;
	}

	void VertexBuffer::UploadData(const void* data, const uint32_t size, const uint32_t offset)
	{
#ifdef FROSTIUM_OPENGL_IMPL
#else
		SetData(data, size, offset);
#endif
	}
}