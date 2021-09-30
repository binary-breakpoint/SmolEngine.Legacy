#include "stdafx.h"
#include "Primitives/IndexBuffer.h"

namespace SmolEngine
{
	void IndexBuffer::Bind() const
	{

	}

	void IndexBuffer::UnBind() const
	{

	}

	void IndexBuffer::Clear()
	{
#ifdef OPENGL_IMPL
#else
		Destroy();
#endif
	}

	bool IndexBuffer::IsReady() const
	{
		return GetSize() > 0;
	}

	void IndexBuffer::UploadData(uint32_t* indices, uint32_t count)
	{
#ifdef OPENGL_IMPL

#else
		SetData(indices, count);
#endif
	}

	uint32_t IndexBuffer::GetCount() const
	{
		return GetElementsCount();
	}

	void IndexBuffer::Create(IndexBuffer* out_ib, uint32_t* indices, uint32_t count, bool is_static)
	{
#ifdef OPENGL_IMPL

		out_ib->Init(indices, count);
#else
		out_ib->Init(indices, count, is_static);
#endif
	}

	void IndexBuffer::Create(IndexBuffer* out_ib, uint32_t size, bool is_static)
	{
#ifdef OPENGL_IMPL
#else
		out_ib->Init(size);
#endif
	}
}