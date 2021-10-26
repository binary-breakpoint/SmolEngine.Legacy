#include "stdafx.h"
#ifndef OPENGL_IMPL
#include "Backends/Vulkan/VulkanACStructure.h"

namespace SmolEngine
{
	bool VulkanACStructure::Build(VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo, VmaMemoryUsage memUsage)
	{
		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = buildSizeInfo.accelerationStructureSize;
		bufferCreateInfo.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

		m_Alloc = VulkanAllocator::AllocBuffer(bufferCreateInfo, memUsage, m_Buffer);
		return true;
	}

	void VulkanACStructure::Free()
	{
		if (m_Alloc != nullptr)
		{
			VulkanAllocator::FreeBuffer(m_Buffer, m_Alloc);

			m_Alloc = nullptr;
			m_Buffer = nullptr;
			m_Handle = nullptr;
			m_DeviceAddress = 0;
		}
	}

	VkAccelerationStructureKHR VulkanACStructure::GetHandle() const
	{
		return m_Handle;
	}
}

#endif