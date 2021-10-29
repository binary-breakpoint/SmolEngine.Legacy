#pragma once
#ifndef OPENGL_IMPL

#include "Backends/Vulkan/Vulkan.h"
#include "Backends/Vulkan/VulkanBuffer.h"

namespace SmolEngine
{
	struct RaytracingPipelineCreateInfo;

	class VulkanACStructure
	{
	public:
		VulkanACStructure() = default;
		~VulkanACStructure();

		void                       Free();
		bool                       BuildBottomLevel(RaytracingPipelineCreateInfo* info);
		VkAccelerationStructureKHR GetHandle() const;

	private:
		bool                       BuildBufferEX(size_t size, VmaMemoryUsage memUsage = VMA_MEMORY_USAGE_GPU_ONLY);

	private:
		VkAccelerationStructureKHR m_Handle = nullptr;
		VmaAllocation              m_Alloc = nullptr;
		VkBuffer                   m_Buffer = nullptr;
		uint64_t                   m_DeviceAddress = 0;
		VulkanBuffer               m_TransformBuffer{};
	};
}

#endif