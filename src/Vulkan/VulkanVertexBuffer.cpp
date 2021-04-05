#include "stdafx.h"
#include "Vulkan/VulkanVertexBuffer.h"

namespace Frostium
{
	void VulkanVertexBuffer::Create(const void* data, uint64_t size, bool is_static)
	{
		CreateBuffer(data, size, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	}

	void VulkanVertexBuffer::Create(uint64_t size)
	{
		CreateBuffer(size, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	}
}