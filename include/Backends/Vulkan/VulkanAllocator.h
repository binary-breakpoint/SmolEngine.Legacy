#pragma once
#ifndef OPENGL_IMPL
#include "Backends/Vulkan/Vulkan.h"


#include <vulkan_memory_allocator/vk_mem_alloc.h>

namespace SmolEngine
{
	class VulkanDevice;
	class VulkanInstance;

	class VulkanAllocator
	{
	public:
		VulkanAllocator();
		~VulkanAllocator();

		void                              Init(VulkanDevice* device, VulkanInstance* instance);
		static VmaAllocation              AllocBuffer(VkBufferCreateInfo ci, VmaMemoryUsage usage, VkBuffer& outBuffer);
		static VmaAllocation              AllocImage(VkImageCreateInfo ci, VmaMemoryUsage usage, VkImage& outImage);
		static void                       AllocFree(VmaAllocation alloc);
		static void                       FreeImage(VkImage image, VmaAllocation allocation);
		static void                       FreeBuffer(VkBuffer buffer, VmaAllocation allocation);
		static void                       UnmapMemory(VmaAllocation allocation);
		static VmaAllocator&              GetAllocator();

		template<typename T>
		static T* MapMemory(VmaAllocation allocation)
		{
			T* mappedMemory;
			vmaMapMemory(VulkanAllocator::GetAllocator(), allocation, (void**)&mappedMemory);
			return mappedMemory;
		}


	private:
		inline static VulkanAllocator*  s_Instance = nullptr;
		VmaAllocator                    m_Allocator;
		uint64_t                        m_TotalAllocatedBytes = 0;
	};
}

#endif