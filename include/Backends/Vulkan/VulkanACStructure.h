#pragma once
#ifndef OPENGL_IMPL

#include "Backends/Vulkan/Vulkan.h"

namespace SmolEngine
{
	class VulkanACStructure
	{
	public:
		VulkanACStructure() = default;
		~VulkanACStructure();

		bool Build(VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo, VmaMemoryUsage memUsage = VMA_MEMORY_USAGE_GPU_ONLY);
		void Free();
		VkAccelerationStructureKHR GetHandle() const;

	private:
		VkAccelerationStructureKHR m_Handle = nullptr;
		VmaAllocation m_Alloc = nullptr;
		VkBuffer m_Buffer = nullptr;
		uint64_t m_DeviceAddress = 0;
	};
}

#endif