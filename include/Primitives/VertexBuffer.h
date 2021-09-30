#pragma once
#include "Primitives/BufferLayout.h"

#ifdef OPENGL_IMPL
#include "Backends/OpenGL/OpenglBuffer.h"
#else
#include "Backends/Vulkan/VulkanVertexBuffer.h"
#endif

namespace SmolEngine
{
#ifdef  OPENGL_IMPL
	class VertexBuffer: OpenglVertexBuffer
#else
	class VertexBuffer: VulkanVertexBuffer
#endif
	{
	public:
		void                  Bind() const;
		void                  UnBind() const;
		void                  Clear();
		bool                  IsReady() const;
		void                  UploadData(const void* data, const uint32_t size, const uint32_t offset = 0);
#ifndef OPENGL_IMPL
		VulkanVertexBuffer&   GetVulkanVertexBuffer() { return *dynamic_cast<VulkanVertexBuffer*>(this); }
#endif
		// Factory
		static void           Create(VertexBuffer* ot_vb, uint32_t size);
		static void           Create (VertexBuffer* ot_vb, void* vertices, uint32_t size, bool is_static = false);
	};
}