#pragma once
#ifndef OPENGL_IMPL
#include "Backends/Vulkan/VulkanBuffer.h"
#include "Primitives/IndexBuffer.h"

namespace SmolEngine
{
	class VulkanIndexBuffer : public IndexBuffer, public VulkanBuffer
	{
	public:
		~VulkanIndexBuffer();

		void  GetBufferFlagsEX(VkBufferUsageFlags& flags);
		bool  BuildFromMemory(uint32_t* indices, size_t count, bool is_static = false) override;
		bool  BuildFromSize(size_t size, bool is_static = false) override;
		void  Update(uint32_t* indices, size_t count, uint32_t offset = 0) override;
		void  Free() override;
		bool  IsGood() const override;
	};
}
#endif