#include "stdafx.h"
#ifndef FROSTIUM_OPENGL_IMPL
#include "Backends/Vulkan/VulkanMemoryAllocator.h"
#include "Backends/Vulkan/VulkanDevice.h"

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	bool VulkanMemoryAllocator::Allocate(VkDevice device, const VkMemoryRequirements& memRequirements, VkDeviceMemory* dest, uint32_t memoryTypeIndex)
	{
		VkMemoryAllocateInfo memAllocateInfo = {};
		{
			memAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			memAllocateInfo.allocationSize = memRequirements.size;
			memAllocateInfo.memoryTypeIndex = memoryTypeIndex;

			VkResult result = vkAllocateMemory(device, &memAllocateInfo, nullptr, dest);
			assert(result == VK_SUCCESS);

			return result == VK_SUCCESS;
		}
	}

	bool VulkanMemoryAllocator::Allocate(const VulkanDevice* device, const VkMemoryRequirements& memRequirements, VkDeviceMemory* dest, VkMemoryPropertyFlags flags)
	{
		VkMemoryAllocateInfo memAllocateInfo = {};
		{
			memAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			memAllocateInfo.allocationSize = memRequirements.size;
			memAllocateInfo.memoryTypeIndex = device->GetMemoryTypeIndex(memRequirements.memoryTypeBits, flags);

			VkResult result = vkAllocateMemory(device->GetLogicalDevice(), &memAllocateInfo, nullptr, dest);
			assert(result == VK_SUCCESS);

			DebugLog::LogInfo("VulkanMemoryAllocator allocated {} bytes of memory", memRequirements.size);
			return result == VK_SUCCESS;
		}
	}
}
#endif