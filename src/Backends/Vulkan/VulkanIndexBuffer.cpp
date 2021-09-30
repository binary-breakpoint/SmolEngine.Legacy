#include "stdafx.h"
#ifndef OPENGL_IMPL
#include "Backends/Vulkan/VulkanIndexBuffer.h"
#include "Backends/Vulkan/VulkanDevice.h"

namespace SmolEngine
{
	void VulkanIndexBuffer::Init(const uint32_t* data, uint64_t count, bool is_static)
	{
		m_ElementsCount = static_cast<uint32_t>(count);
		if (is_static)
		{
			CreateStaticBuffer(data, sizeof(uint32_t) * count,
				VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

			return;
		}

		CreateBuffer(data, sizeof(uint32_t) * count,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	}

	void VulkanIndexBuffer::Init(uint64_t size)
	{
		CreateBuffer(size, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	}

	uint32_t VulkanIndexBuffer::GetElementsCount() const
	{
		return m_ElementsCount;
	}
}
#endif