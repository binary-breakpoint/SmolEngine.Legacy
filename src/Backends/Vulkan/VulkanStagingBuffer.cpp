#include "stdafx.h"
#ifndef FROSTIUM_OPENGL_IMPL
#include "Backends/Vulkan/VulkanStagingBuffer.h"

namespace SmolEngine
{
	void VulkanStagingBuffer::Create(const void* data, uint64_t size)
	{
		CreateBuffer(data, size, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	}

	void VulkanStagingBuffer::Create(uint64_t size)
	{
		CreateBuffer(size, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	}
}
#endif