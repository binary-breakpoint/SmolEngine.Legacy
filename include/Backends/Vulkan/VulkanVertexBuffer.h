#pragma once
#ifndef OPENGL_IMPL
#include "Backends/Vulkan/VulkanBuffer.h"

namespace SmolEngine
{
	class VulkanVertexBuffer : public VulkanBuffer
	{
	public:
		VulkanVertexBuffer() = default;
		~VulkanVertexBuffer() = default;

		void Init(const void* data, uint64_t size, bool is_static);
		void Init(uint64_t size);
	};
}
#endif