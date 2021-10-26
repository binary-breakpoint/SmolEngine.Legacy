#include "stdafx.h"
#ifndef OPENGL_IMPL
#include "Backends/Vulkan/VulkanVertexBuffer.h"
#include "Backends/Vulkan/VulkanContext.h"

namespace SmolEngine
{
	VulkanVertexBuffer::~VulkanVertexBuffer()
	{
		Free();
	}

	void VulkanVertexBuffer::GetBufferStateEX(bool& deviceAdress, VkBufferUsageFlags& flags)
	{
		flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		deviceAdress = false;

		if (VulkanContext::GetDevice().GetRaytracingSupport())
		{
			deviceAdress = true;
			flags |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
		}
	}

	bool VulkanVertexBuffer::BuildFromMemory(void* vertices, size_t size, bool is_static)
	{
		VkBufferUsageFlags usage;
		bool deviceAdress;
		GetBufferStateEX(deviceAdress, usage);

		if (is_static){ CreateStaticBuffer(vertices, size, usage, deviceAdress); }
		else { CreateBuffer(vertices, size, usage, deviceAdress); }

		m_VertexCount = static_cast<uint32_t>(size);
		return true;
	}

	bool VulkanVertexBuffer::BuildFromSize(size_t size, bool is_static)
	{
		VkBufferUsageFlags usage;
		bool deviceAdress;
		GetBufferStateEX(deviceAdress, usage);

		CreateBuffer(size, usage, deviceAdress);
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