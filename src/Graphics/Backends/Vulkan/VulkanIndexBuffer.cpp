#include "stdafx.h"
#ifndef OPENGL_IMPL
#include "Graphics/Backends/Vulkan/VulkanIndexBuffer.h"
#include "Graphics/Backends/Vulkan/VulkanContext.h"

namespace SmolEngine
{
	VulkanIndexBuffer::~VulkanIndexBuffer()
	{
		Free();
	}

	void VulkanIndexBuffer::GetBufferFlagsEX(VkBufferUsageFlags& flags)
	{
		flags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		if (VulkanContext::GetDevice().GetRaytracingSupport())
		{
			flags |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
		}
	}

	bool VulkanIndexBuffer::BuildFromMemory(uint32_t* indices, size_t count, bool is_static)
	{
		VkBufferUsageFlags usage;
		GetBufferFlagsEX(usage);

		if (is_static) { CreateStaticBuffer(indices, sizeof(uint32_t) * count, usage); }
		else { CreateBuffer(indices, sizeof(uint32_t) * count, usage); }

		m_Elements = static_cast<uint32_t>(count);
		return true;
	}

	bool VulkanIndexBuffer::BuildFromSize(size_t size, bool is_static)
	{
		VkBufferUsageFlags usage;
		GetBufferFlagsEX(usage);

		CreateBuffer(size, usage);
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