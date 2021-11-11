#pragma once
#ifndef OPENGL_IMPL

#include "Graphics/Backends/Vulkan/Vulkan.h"

namespace SmolEngine
{
	class VulkanDevice;

	class VulkanBuffer
	{
	public:
		VulkanBuffer();
		virtual ~VulkanBuffer();

		void*                   MapMemory();
		void                    UnMapMemory();
		void                    Destroy();
		void                    CreateBuffer(const void* data, size_t size, VkBufferUsageFlags bufferUsage, VmaMemoryUsage VmaUsage = VMA_MEMORY_USAGE_CPU_TO_GPU);
		void                    CreateBuffer(size_t size, VkBufferUsageFlags bufferUsage, VmaMemoryUsage VmaUsage = VMA_MEMORY_USAGE_CPU_TO_GPU);
		void                    CreateStaticBuffer(const void* data, size_t size, VkBufferUsageFlags usageFlags);
		void                    SetData(const void* data, size_t size, uint32_t offset = 0);
		size_t                  GetSize() const;
		const VkBuffer&         GetBuffer() const;

	private:
		void*                   m_Mapped = nullptr;
		VkBuffer                m_Buffer = nullptr;
		VmaAllocation           m_Alloc = nullptr;
		VulkanDevice*           m_Device = nullptr;
		size_t                  m_Size = 0;
	};
}
#endif