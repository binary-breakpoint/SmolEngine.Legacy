#include "stdafx.h"

#ifndef OPENGL_IMPL
#include "Backends/Vulkan/VulkanScratchBuffer.h"
#include "Backends/Vulkan/VulkanContext.h"

namespace SmolEngine
{
	VulkanScratchBuffer::~VulkanScratchBuffer()
	{
		Free();
	}

	void VulkanScratchBuffer::Free()
	{
		if (m_Alloc != nullptr)
		{
			VulkanAllocator::FreeBuffer(m_Buffer, m_Alloc);

			m_Alloc = nullptr;
			m_Buffer = nullptr;
			m_DeviceAddress = 0;
		}
	}

	void VulkanScratchBuffer::Build(size_t size, VmaMemoryUsage memUsage)
	{
		VkDeviceSize bufferSize = size;

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = bufferSize;
		bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

		m_Alloc = VulkanAllocator::AllocBuffer(bufferInfo, memUsage, m_Buffer);

		VkBufferDeviceAddressInfoKHR bufferDeviceAddressInfo{};
		bufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		bufferDeviceAddressInfo.buffer = m_Buffer;

		m_DeviceAddress = vkGetBufferDeviceAddressKHR(VulkanContext::GetDevice().GetLogicalDevice(), &bufferDeviceAddressInfo);
	}
}

#endif