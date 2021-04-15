#include "stdafx.h"
#ifndef FROSTIUM_OPENGL_IMPL
#include "Vulkan/VulkanBuffer.h"

#include "Vulkan/VulkanContext.h"
#include "Vulkan/VulkanStagingBuffer.h"

namespace Frostium
{
	VulkanBuffer::VulkanBuffer()
	{
		m_Device = &VulkanContext::GetDevice();
	}

	VulkanBuffer::~VulkanBuffer()
	{
		Destroy();
	}

	void VulkanBuffer::CreateBuffer(const void* data, size_t size, VkMemoryPropertyFlags memFlags, VkBufferUsageFlags usageFlags, uint32_t offset, VkSharingMode shareMode)
	{
		const auto& device = m_Device->GetLogicalDevice();

		VkDeviceSize bufferSize = size;
		VkBufferCreateInfo bufferInfo = {};
		{
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = bufferSize;
			bufferInfo.usage = usageFlags;
			bufferInfo.sharingMode = shareMode;

			VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, &m_Buffer);
			assert(result == VK_SUCCESS);
		}

		assert(m_Buffer != VK_NULL_HANDLE);

		VkMemoryRequirements memoryRequirements;
		vkGetBufferMemoryRequirements(device, m_Buffer, &memoryRequirements);
		m_MemoryType = FindMemoryType(memoryRequirements.memoryTypeBits, memFlags);
		m_MemoryRequirementsSize = memoryRequirements.size;
		m_Alignment = memoryRequirements.alignment;
		m_Size = size;
		m_UsageFlags = usageFlags;
		m_MemFlags = memFlags;

		VkMemoryAllocateInfo memAllocateInfo = {};
		{
			memAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			memAllocateInfo.allocationSize = memoryRequirements.size;
			memAllocateInfo.memoryTypeIndex = m_MemoryType;

			VkResult result = vkAllocateMemory(device, &memAllocateInfo, nullptr, &m_DeviceMemory);
			assert(result == VK_SUCCESS);

#ifdef Frostium_DEBUG
			NATIVE_INFO("VulkanBuffer:: Allocating {} bytes of memory", size);
#endif // Frostium_DEBUG

		}

		if (data)
		{
			void* destBuffer = nullptr;
			VkResult map_result = vkMapMemory(device, m_DeviceMemory, 0, m_MemoryRequirementsSize, 0, &destBuffer);
			assert(map_result == VK_SUCCESS);

			memcpy(destBuffer, data, size);
			vkUnmapMemory(device, m_DeviceMemory);
		}

		VkResult bind_result = vkBindBufferMemory(device, m_Buffer, m_DeviceMemory, offset);
		assert(bind_result == VK_SUCCESS);
	}

	void VulkanBuffer::CreateBuffer(size_t size, VkMemoryPropertyFlags memFlags, VkBufferUsageFlags usageFlags,
		uint32_t offset, VkSharingMode shareMode, VkMemoryAllocateFlagsInfo* allFlagsInfo)
	{
		const auto& device = m_Device->GetLogicalDevice();

		VkDeviceSize bufferSize = size;
		VkBufferCreateInfo bufferInfo = {};
		{
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = bufferSize;
			bufferInfo.usage = usageFlags;
			bufferInfo.sharingMode = shareMode;

			VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, &m_Buffer);
			assert(result == VK_SUCCESS);
		}

		assert(m_Buffer != VK_NULL_HANDLE);

		VkMemoryRequirements memoryRequirements;
		vkGetBufferMemoryRequirements(device, m_Buffer, &memoryRequirements);
		m_MemoryType = FindMemoryType(memoryRequirements.memoryTypeBits, memFlags);
		m_MemoryRequirementsSize = memoryRequirements.size;
		m_Size = size;
		m_UsageFlags = usageFlags;
		m_MemFlags = memFlags;

		VkMemoryAllocateInfo memAllocateInfo = {};
		{
			memAllocateInfo.pNext = allFlagsInfo;
			memAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			memAllocateInfo.allocationSize = memoryRequirements.size;
			memAllocateInfo.memoryTypeIndex = m_MemoryType;

			VkResult result = vkAllocateMemory(device, &memAllocateInfo, nullptr, &m_DeviceMemory);
			assert(result == VK_SUCCESS);

#ifdef FROSTIUM_DEBUG
			NATIVE_INFO("VulkanBuffer:: Allocating {} bytes of memory", size);
#endif

		}

		VkResult bind_result = vkBindBufferMemory(device, m_Buffer, m_DeviceMemory, offset);
		assert(bind_result == VK_SUCCESS);
	}

	void VulkanBuffer::CreateStaticBuffer(const void* data, size_t size, VkBufferUsageFlags usageFlags)
	{
		VulkanStagingBuffer staging = {};
		staging.Create(data, size);
		CreateBuffer(nullptr, size, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, usageFlags);

		VkCommandBuffer copyCmd = VulkanCommandBuffer::CreateSingleCommandBuffer();
		{
			VkBufferCopy copyRegion = { };
			copyRegion.size = m_Size;
			vkCmdCopyBuffer(
				copyCmd,
				staging.GetBuffer(),
				m_Buffer,
				1,
				&copyRegion);
		}
		VulkanCommandBuffer::FlushCommandBuffer(copyCmd);
	}

	void VulkanBuffer::Flush() const
	{
		VkMappedMemoryRange mappedRange = {};
		mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		mappedRange.memory = m_DeviceMemory;
		mappedRange.offset = 0;
		mappedRange.size = VK_WHOLE_SIZE;
		
		const auto& device = m_Device->GetLogicalDevice();
		vkFlushMappedMemoryRanges(device, 1, &mappedRange);
	}

	void VulkanBuffer::Destroy()
	{
		const auto& device = m_Device->GetLogicalDevice();
		if (m_Buffer != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(device, m_Buffer, nullptr);
			m_Buffer = VK_NULL_HANDLE;
		}

		if (m_DeviceMemory != VK_NULL_HANDLE)
		{
			vkFreeMemory(device, m_DeviceMemory, nullptr);
			m_DeviceMemory = VK_NULL_HANDLE;
		}
	}

	void VulkanBuffer::SetData(const void* data, size_t size, uint32_t offset)
	{
		if (m_Buffer == VK_NULL_HANDLE)
		{
			CreateBuffer(data, size, m_MemFlags, m_UsageFlags, offset);
			return;
		}

		if (m_Mapped == nullptr)
		{
			uint8_t* dest = nullptr;
			const auto& device = m_Device->GetLogicalDevice();
			VK_CHECK_RESULT(vkMapMemory(device, m_DeviceMemory, 0, m_MemoryRequirementsSize, 0, (void**)&dest));

			if (!dest)
				assert(false);

			memcpy(dest, data, size);
			UnMapMemory();
			return;
		}

		memcpy(m_Mapped, data, size);
	}

	void VulkanBuffer::CmdUpdateData(VkCommandBuffer cmdBuffer, const void* data, size_t size, uint32_t offset)
	{
		vkCmdUpdateBuffer(cmdBuffer, m_Buffer, offset, size, data);
	}

	void* VulkanBuffer::MapMemory()
	{
		uint8_t* data;
		const auto& device = m_Device->GetLogicalDevice();
		VK_CHECK_RESULT(vkMapMemory(device, m_DeviceMemory, 0, m_MemoryRequirementsSize, 0, (void**)&data));
		m_Mapped = data;
		return data;
	}

	void VulkanBuffer::UnMapMemory()
	{
		const auto& device = m_Device->GetLogicalDevice();
		vkUnmapMemory(device, m_DeviceMemory);
		m_Mapped = nullptr;
	}

	size_t VulkanBuffer::GetSize() const
	{
		return m_Size;
	}

	const VkBuffer VulkanBuffer::GetBuffer() const
	{
		return m_Buffer;
	}

	const VkDevice VulkanBuffer::GetDevice() const
	{
		return m_Device->GetLogicalDevice();
	}

	const VkDeviceMemory VulkanBuffer::GetDeviceMemory() const
	{
		return m_DeviceMemory;
	}

	uint32_t VulkanBuffer::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags memFlags)
	{
		const auto& device_mem = *VulkanContext::GetDevice().GetMemoryProperties();

		// Iterate over all memory types available for the device used in this example
		for (uint32_t i = 0; i < device_mem.memoryHeapCount; i++)
		{
			if ((typeFilter & 1) == 1)
			{
				if ((device_mem.memoryTypes[i].propertyFlags & memFlags) == memFlags)
				{
					return i;
				}
			}

			typeFilter >>= 1;
		}

		return -1;
	}
}
#endif