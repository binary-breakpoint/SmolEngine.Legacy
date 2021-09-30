#pragma once
#include "Primitives/BufferLayout.h"

#ifdef OPENGL_IMPL
#include "Backends/OpenGL/OpenglBuffer.h"
#else
#include "Backends/Vulkan/VulkanIndexBuffer.h"
#endif

namespace SmolEngine
{
#ifdef  OPENGL_IMPL
	class IndexBuffer: OpenglIndexBuffer
#else
	class IndexBuffer: VulkanIndexBuffer
#endif
	{
	public:
		void                  Bind() const;
		void                  UnBind() const;
		void                  Clear();
		bool                  IsReady() const;
		void                  UploadData(uint32_t* indices, uint32_t count);
		uint32_t              GetCount() const;
#ifndef OPENGL_IMPL
		VulkanIndexBuffer&    GetVulkanIndexBuffer() { return *dynamic_cast<VulkanIndexBuffer*>(this); };
#endif
		static void           Create(IndexBuffer* out_ib, uint32_t* indices, uint32_t count, bool is_static = false);
		static void           Create(IndexBuffer* out_ib, uint32_t size, bool is_static = false);
	};
}