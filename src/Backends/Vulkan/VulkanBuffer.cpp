#include "stdafx.h"
#ifndef OPENGL_IMPL
#include "Backends/Vulkan/VulkanBuffer.h"
#include "Backends/Vulkan/VulkanContext.h"
#include "Backends/Vulkan/VulkanStagingBuffer.h"

namespace SmolEngine
{
	VulkanBuffer::VulkanBuffer()
	{
		m_Device = &VulkanContext::GetDevice();
	}

	VulkanBuffer::~VulkanBuffer()
	{
		Destroy();
	}

	void VulkanBuffer::CreateBuffer(const void* data, size_t size, VkBufferUsageFlags bufferUsage, bool deviceAdress, VmaMemoryUsage VmaUsage)
	{
		const auto& device = m_Device->GetLogicalDevice();

		VkDeviceSize bufferSize = size;
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = bufferSize;
		bufferInfo.usage = bufferUsage;
		bufferInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;

		m_Alloc = VulkanAllocator::AllocBuffer(bufferInfo, VmaUsage, m_Buffer, deviceAdress);
		m_Size = size;

		if (data)
			SetData(data, size);
	}

	void VulkanBuffer::CreateBuffer(size_t size, VkBufferUsageFlags bufferUsage, bool deviceAdress, VmaMemoryUsage VmaUsage)
	{
		const auto& device = m_Device->GetLogicalDevice();

		VkDeviceSize bufferSize = size;
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = bufferSize;
		bufferInfo.usage = bufferUsage;
		bufferInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;

		m_Alloc = VulkanAllocator::AllocBuffer(bufferInfo, VmaUsage, m_Buffer, deviceAdress);
		m_Size = size;
	}

	void VulkanBuffer::CreateStaticBuffer(const void* data, size_t size, VkBufferUsageFlags usageFlags, bool deviceAdress)
	{
		VulkanStagingBuffer staging = {};
		staging.Create(data, size);

		CreateBuffer(nullptr, size, usageFlags, deviceAdress, VMA_MEMORY_USAGE_GPU_ONLY);

		CommandBufferStorage cmdStorage{};
		VulkanCommandBuffer::CreateCommandBuffer(&cmdStorage);
		{
			VkBufferCopy copyRegion = { };
			copyRegion.size = m_Size;
			vkCmdCopyBuffer(
				cmdStorage.Buffer,
				staging.GetBuffer(),
				m_Buffer,
				1,
				&copyRegion);
		}
		VulkanCommandBuffer::ExecuteCommandBuffer(&cmdStorage);
	}

	void VulkanBuffer::Destroy()
	{
		if (m_Alloc != nullptr)
		{
			VulkanAllocator::FreeBuffer(m_Buffer, m_Alloc);
			m_Alloc = nullptr;
		}
	}

	void VulkanBuffer::SetData(const void* data, size_t size, uint32_t offset)
	{
		void* dest = MapMemory();
		{
			memcpy(dest, data, size);
		}
		UnMapMemory();
	}

	void VulkanBuffer::CmdUpdateData(VkCommandBuffer cmdBuffer, const void* data, size_t size, uint32_t offset)
	{
		vkCmdUpdateBuffer(cmdBuffer, m_Buffer, offset, size, data);
	}

	void* VulkanBuffer::MapMemory()
	{
		uint8_t* destData = VulkanAllocator::MapMemory<uint8_t>(m_Alloc);
		m_Mapped = destData;
		return m_Mapped;
	}

	void VulkanBuffer::UnMapMemory()
	{
		if (m_Mapped != nullptr)
		{
			VulkanAllocator::UnmapMemory(m_Alloc);
			m_Mapped = nullptr;
		}
	}

	size_t VulkanBuffer::GetSize() const
	{
		return m_Size;
	}

	const VkBuffer& VulkanBuffer::GetBuffer() const
	{
		return m_Buffer;
	}
}
#endif