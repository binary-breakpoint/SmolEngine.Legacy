#include "stdafx.h"
#ifndef OPENGL_IMPL
#include "Backends/Vulkan/VulkanStagingBuffer.h"

namespace SmolEngine
{
	void VulkanStagingBuffer::Create(const void* data, uint64_t size)
	{
		CreateBuffer(data, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	}

	void VulkanStagingBuffer::Create(uint64_t size)
	{
		CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
	}
}
#endif