#include "stdafx.h"
#ifndef OPENGL_IMPL
#include "Backends/Vulkan/VulkanAllocator.h"
#include "Backends/Vulkan/VulkanDevice.h"
#include "Backends/Vulkan/VulkanInstance.h"
#include "Common/DebugLog.h"

namespace SmolEngine
{
	VulkanAllocator::VulkanAllocator()
	{
		s_Instance = this;
	}

	VulkanAllocator::~VulkanAllocator()
	{
		vmaDestroyAllocator(m_Allocator);
		s_Instance = nullptr;
	}

	void VulkanAllocator::Init(VulkanDevice* device, VulkanInstance* instance)
	{
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_2;
		allocatorInfo.physicalDevice = device->GetPhysicalDevice();
		allocatorInfo.device = device->GetLogicalDevice();
		allocatorInfo.instance = instance->GetInstance();

		if(device->GetRaytracingSupport())
			allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

		vmaCreateAllocator(&allocatorInfo, &m_Allocator);
	}

	VmaAllocation VulkanAllocator::AllocBuffer(VkBufferCreateInfo ci, VmaMemoryUsage usage, VkBuffer& outBuffer)
	{
		VmaAllocationCreateInfo allocCreateInfo = {};
		allocCreateInfo.usage = usage;

		VmaAllocation allocation;
		vmaCreateBuffer(s_Instance->m_Allocator, &ci, &allocCreateInfo, &outBuffer, &allocation, nullptr);

		VmaAllocationInfo allocInfo{};
		vmaGetAllocationInfo(s_Instance->m_Allocator, allocation, &allocInfo);

#ifdef SMOLENGINE_DEBUG

		s_Instance->m_TotalAllocatedBytes += allocInfo.size;

		DebugLog::LogInfo("VulkanAllocator: allocating buffer; size = {}", allocInfo.size);
		DebugLog::LogInfo("VulkanAllocator: total allocated since start is = {}", s_Instance->m_TotalAllocatedBytes);

#endif 

		return allocation;
	}

	VmaAllocation VulkanAllocator::AllocImage(VkImageCreateInfo ci, VmaMemoryUsage usage, VkImage& outImage)
	{
		VmaAllocationCreateInfo allocCreateInfo = {};
		allocCreateInfo.usage = usage;

		VmaAllocation allocation;
		vmaCreateImage(s_Instance->m_Allocator, &ci, &allocCreateInfo, &outImage, &allocation, nullptr);
		VmaAllocationInfo allocInfo;
		vmaGetAllocationInfo(s_Instance->m_Allocator, allocation, &allocInfo);

#ifdef SMOLENGINE_DEBUG
		s_Instance->m_TotalAllocatedBytes += allocInfo.size;
		DebugLog::LogInfo("VulkanAllocator: allocating image; size = {}", allocInfo.size);
		DebugLog::LogInfo("VulkanAllocator: total allocated since start is = {}", s_Instance->m_TotalAllocatedBytes);
#endif // SMOLENGINE_DEBUG
		return allocation;
	}

	void VulkanAllocator::AllocFree(VmaAllocation alloc)
	{
		vmaFreeMemory(s_Instance->m_Allocator, alloc);
	}

	void VulkanAllocator::FreeImage(VkImage image, VmaAllocation allocation)
	{
		vmaDestroyImage(s_Instance->m_Allocator, image, allocation);
	}

	void VulkanAllocator::FreeBuffer(VkBuffer buffer, VmaAllocation allocation)
	{
		vmaDestroyBuffer(s_Instance->m_Allocator, buffer, allocation);
	}

	void VulkanAllocator::UnmapMemory(VmaAllocation allocation)
	{
		vmaUnmapMemory(s_Instance->m_Allocator, allocation);
	}

	VmaAllocator& VulkanAllocator::GetAllocator()
	{
		return s_Instance->m_Allocator;
	}
}
#endif