#include "stdafx.h"
#include "Primitives/VertexArray.h"

namespace SmolEngine
{
	void VertexArray::Bind() const
	{
#ifdef FROSTIUM_OPENGL_IMPL
		m_OpenglVertexArray.Bind();
#endif
	}

	void VertexArray::UnBind() const
	{
#ifdef FROSTIUM_OPENGL_IMPL
		m_OpenglVertexArray.UnBind();
#endif
	}

	void VertexArray::SetVertexBuffer(const Ref<VertexBuffer>& vertexBuffer)
	{
#ifdef FROSTIUM_OPENGL_IMPL
		m_OpenglVertexArray.SetVertexBuffer(vertexBuffer);
#endif
	}

	void VertexArray::SetIndexBuffer(const Ref<IndexBuffer>& indexBuffer)
	{
#ifdef FROSTIUM_OPENGL_IMPL
		m_OpenglVertexArray.SetIndexBuffer(indexBuffer);
#endif

	}

	Ref<IndexBuffer> VertexArray::GetIndexBuffer() const
	{
#ifdef FROSTIUM_OPENGL_IMPL
		return m_OpenglVertexArray.GetIndexBuffer();
#else
		return nullptr;
#endif

	}

	Ref<VertexArray> VertexArray::Create()
	{
		auto vertexArray = std::make_shared<VertexArray>();

#ifdef FROSTIUM_OPENGL_IMPL
		vertexArray->m_OpenglVertexArray.Init();
#endif
		return vertexArray;
	}
}