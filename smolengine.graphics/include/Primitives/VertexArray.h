#pragma once
#include "Memory.h"

#include "Common/BufferLayout.h"
#include "Primitives/VertexBuffer.h"
#include "Primitives/IndexBuffer.h"

#include <vector>
#include <string>
#include <memory>

#ifdef OPENGL_IMPL
#include "OpenGL/OpenglBuffer.h"
#include "OpenGL/OpenglVertexArray.h"
#endif

namespace SmolEngine
{
	class VertexArray
	{
	public:

		VertexArray() = default;
		~VertexArray() = default;

		// Binding
		void Bind() const;
		void UnBind() const;

		// Setters
		void SetVertexBuffer(const Ref<VertexBuffer>& vertexBuffer);
		void SetIndexBuffer(const Ref<IndexBuffer>& indexBuffer);

		// Getters
		Ref<IndexBuffer> GetIndexBuffer() const;

		// Factory
		static Ref<VertexArray> Create();

	private:

#ifdef  OPENGL_IMPL
		OpenglVertexArray m_OpenglVertexArray = {};
#endif

	};
}