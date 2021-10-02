#include "stdafx.h"
#ifndef OPENGL_IMPL
#include "Backends/Vulkan/VulkanVertexBuffer.h"

namespace SmolEngine
{
	VulkanVertexBuffer::~VulkanVertexBuffer()
	{
		Free();
	}

	bool VulkanVertexBuffer::BuildFromMemory(void* vertices, size_t size, bool is_static)
	{
		if (is_static)
		{
			CreateStaticBuffer(vertices, size,
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
		}
		else
		{
			CreateBuffer(vertices, size, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
		}

		return true;
	}

	bool VulkanVertexBuffer::BuildFromSize(size_t size, bool is_static)
	{
		CreateBuffer(size, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

		return true;
	}

	void VulkanVertexBuffer::Free()
	{
		Destroy();
	}

	bool VulkanVertexBuffer::IsGood() const
	{
		return GetSize() > 0;
	}

	void VulkanVertexBuffer::Update(const void* data, size_t size, const uint32_t offset)
	{
		SetData(data, size, offset);
	}
}
#endif