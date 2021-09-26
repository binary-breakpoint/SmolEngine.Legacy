#include "stdafx.h"
#ifndef FROSTIUM_OPENGL_IMPL
#include "Backends/Vulkan/Raytracing/VulkanAccelerationStructure.h"
#include "Backends/Vulkan/VulkanContext.h"

namespace SmolEngine
{
	VulkanAccelerationStructure::~VulkanAccelerationStructure()
	{
		Delete();
	}

	void VulkanAccelerationStructure::Create(VkAccelerationStructureTypeKHR type, VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo)
	{
		m_Device = VulkanContext::GetDevice().GetLogicalDevice();

		// Buffer and memory
		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = buildSizeInfo.accelerationStructureSize;
		bufferCreateInfo.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		VK_CHECK_RESULT(vkCreateBuffer(m_Device, &bufferCreateInfo, nullptr, &m_Buffer));
		VkMemoryRequirements memoryRequirements{};
		vkGetBufferMemoryRequirements(m_Device, m_Buffer, &memoryRequirements);
		VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo{};
		memoryAllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
		memoryAllocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
		VkMemoryAllocateInfo memoryAllocateInfo{};
		memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryAllocateInfo.pNext = &memoryAllocateFlagsInfo;
		memoryAllocateInfo.allocationSize = memoryRequirements.size;
		memoryAllocateInfo.memoryTypeIndex = VulkanContext::GetDevice().GetMemoryTypeIndex(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(m_Device, &memoryAllocateInfo, nullptr, &m_Memory));
		VK_CHECK_RESULT(vkBindBufferMemory(m_Device, m_Buffer, m_Memory, 0));

		// Acceleration structure
		VkAccelerationStructureCreateInfoKHR accelerationStructureCreate_info{};
		accelerationStructureCreate_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		accelerationStructureCreate_info.buffer = m_Buffer;
		accelerationStructureCreate_info.size = buildSizeInfo.accelerationStructureSize;
		accelerationStructureCreate_info.type = type;
		vkCreateAccelerationStructureKHR(m_Device, &accelerationStructureCreate_info, nullptr, &m_Structure);

		// AS device address
		VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
		accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		accelerationDeviceAddressInfo.accelerationStructure = m_Structure;
		m_DeviceAddress = vkGetAccelerationStructureDeviceAddressKHR(m_Device, &accelerationDeviceAddressInfo);
	}

	void VulkanAccelerationStructure::Delete()
	{
		if (m_Device != nullptr)
		{
			if(m_Memory)
				vkFreeMemory(m_Device, m_Memory, nullptr);
			if(m_Buffer)
				vkDestroyBuffer(m_Device, m_Buffer, nullptr);
			if(m_Structure)
				vkDestroyAccelerationStructureKHR(m_Device, m_Structure, nullptr);
		}
	}

	uint64_t VulkanAccelerationStructure::GetDeviceAddress() const
	{
		return m_DeviceAddress;
	}
}
#endif