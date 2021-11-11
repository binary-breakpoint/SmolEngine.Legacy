#pragma once
#include "Graphics/Common/BufferLayout.h"
#include "Graphics/Primitives/PrimitiveBase.h"

namespace SmolEngine
{
	class IndexBuffer: public PrimitiveBase
	{
	public:
		virtual ~IndexBuffer() = default;

		virtual bool                  BuildFromMemory(uint32_t* indices, size_t count, bool is_static = false) = 0;
		virtual bool                  BuildFromSize(size_t size, bool is_static = false) = 0;
		virtual void                  Update(uint32_t* indices, size_t count, uint32_t offset = 0) = 0;
		uint32_t                      GetCount() const;
		static Ref<IndexBuffer>       Create();
		
	protected:
		uint32_t m_Elements = 0;
	};
}