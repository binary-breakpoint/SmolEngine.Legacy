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
		uint32_t                      GetVertexCount() const { return m_VertexCount; }
		static Ref<VertexBuffer>      Create();

	private:
		uint32_t m_VertexCount = 0;

		friend class VulkanVertexBuffer;
	};
}