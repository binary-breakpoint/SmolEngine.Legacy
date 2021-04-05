#pragma once
#include "Common/Core.h"
#include "Vulkan/Vulkan.h"

namespace Frostium
{
	class VulkanDevice;

	class VulkanBuffer
	{
	public:

		VulkanBuffer();

		virtual ~VulkanBuffer();

		void CreateBuffer(const void* data, size_t size, VkMemoryPropertyFlags memFlags, VkBufferUsageFlags usageFlags, uint32_t offset = 0,
			VkSharingMode shareMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE);

		void CreateBuffer(size_t size, VkMemoryPropertyFlags memFlags, VkBufferUsageFlags usageFlags, uint32_t offset = 0,
			VkSharingMode shareMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE);

		void Flush(VkDeviceSize size, VkDeviceSize offset);

		void Destroy();


		void SetData(const void* data, size_t size, uint32_t offset = 0);

		void CmdUpdateData(VkCommandBuffer cmdBuffer, const void* data, size_t size, uint32_t offset = 0);


		void* MapMemory();

		void UnMapMemory();


		size_t GetSize() const;

		const VkBuffer& GetBuffer() const;

		const VkDeviceMemory GetDeviceMemory() const;


		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags memFlags);

	private:

		VkBuffer           m_Buffer = VK_NULL_HANDLE;
		VkDeviceMemory     m_DeviceMemory = VK_NULL_HANDLE;
		VkDeviceSize       m_MemoryRequirementsSize = 0;
		VkDeviceSize       m_Alignment = 0;

		uint32_t           m_MemoryType = UINT32_MAX;
		size_t             m_Size = 0;

		void*              m_Mapped = nullptr;
		VulkanDevice*      m_Device = nullptr;
	};
}