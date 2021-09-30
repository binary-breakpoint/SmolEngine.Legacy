#include "stdafx.h"
#ifndef OPENGL_IMPL
#include "Backends/Vulkan/Raytracing/VulkanScratchBuffer.h"

namespace SmolEngine
{
	VulkanScratchBuffer::~VulkanScratchBuffer()
	{

	}

	void VulkanScratchBuffer::Create(size_t size)
	{
		uint32_t offset = 0;
		VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo = {};
		memoryAllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
		memoryAllocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
		CreateBuffer(size, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			offset, VK_SHARING_MODE_EXCLUSIVE, &memoryAllocateFlagsInfo);

		// Buffer device address
		VkBufferDeviceAddressInfoKHR bufferDeviceAddresInfo{};
		bufferDeviceAddresInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		bufferDeviceAddresInfo.buffer = GetBuffer();
		m_DeviceAddress = vkGetBufferDeviceAddressKHR(GetDevice(), &bufferDeviceAddresInfo);
	}

	uint64_t VulkanScratchBuffer::GetDeviceAddress()
	{
		return m_DeviceAddress;
	}
}

#endif