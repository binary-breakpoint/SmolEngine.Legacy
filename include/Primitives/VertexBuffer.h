#pragma once
#include "Primitives/BufferLayout.h"
#include "Primitives/PrimitiveBase.h"

namespace SmolEngine
{
	class VertexBuffer: public PrimitiveBase
	{
	public:
		virtual ~VertexBuffer() = default;

		virtual bool                  BuildFromMemory(void* vertices, size_t size, bool is_static = false) = 0;
		virtual bool                  BuildFromSize(size_t size, bool is_static = false) = 0;
		virtual void                  Update(const void* data, size_t size, const uint32_t offset = 0) = 0;
		static Ref<VertexBuffer>      Create();
	};
}