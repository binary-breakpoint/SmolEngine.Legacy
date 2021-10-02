#include "stdafx.h"
#ifndef OPENGL_IMPL
#include "Backends/Vulkan/VulkanIndexBuffer.h"
#include "Backends/Vulkan/VulkanDevice.h"

namespace SmolEngine
{
	VulkanIndexBuffer::~VulkanIndexBuffer()
	{
		Free();
	}

	bool VulkanIndexBuffer::BuildFromMemory(uint32_t* indices, size_t count, bool is_static)
	{
		if (is_static)
		{
			CreateStaticBuffer(indices, sizeof(uint32_t) * count,
				VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
		}
		else
		{
			CreateBuffer(indices, sizeof(uint32_t) * count,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
		}

		m_Elements = static_cast<uint32_t>(count);
		return true;
	}

	bool VulkanIndexBuffer::BuildFromSize(size_t size, bool is_static)
	{
		CreateBuffer(size, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

		return true;
	}

	void VulkanIndexBuffer::Update(uint32_t* indices, size_t count, uint32_t offset)
	{
		SetData(indices, sizeof(uint32_t) * count, offset);
	}

	void VulkanIndexBuffer::Free()
	{
		Destroy();
	}

	bool VulkanIndexBuffer::IsGood() const
	{
		return GetSize() > 0;
	}
}
#endif