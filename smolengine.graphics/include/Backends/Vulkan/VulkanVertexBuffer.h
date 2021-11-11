#pragma once
#ifndef OPENGL_IMPL
#include "Backends/Vulkan/VulkanBuffer.h"
#include "Primitives/VertexBuffer.h"

namespace SmolEngine
{
	class VulkanVertexBuffer : public VertexBuffer, public VulkanBuffer
	{
	public:
		~VulkanVertexBuffer();

		bool  BuildFromMemory(void* vertices, size_t size, bool is_static = false) override;
		bool  BuildFromSize(size_t size, bool is_static = false) override;
		void  Free() override;
		bool  IsGood() const override;
		void  Update(const void* data, size_t size, const uint32_t offset = 0) override;

		void  GetBufferFlagsEX(VkBufferUsageFlags& flags);
	};
}
#endif