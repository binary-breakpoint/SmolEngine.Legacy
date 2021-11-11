#pragma once
#ifndef OPENGL_IMPL
#include "Backends/Vulkan/Vulkan.h"
#include "Backends/Vulkan/VulkanBuffer.h"

namespace SmolEngine
{
	class VulkanScratchBuffer
	{
	public:
		VulkanScratchBuffer() = default;
		~VulkanScratchBuffer();

		void     Free();
		void     Build(size_t size, VmaMemoryUsage memUsage = VMA_MEMORY_USAGE_GPU_ONLY);
		uint64_t GetDeviceAddress() const;

	private:
		VmaAllocation m_Alloc = nullptr;
		VkBuffer m_Buffer = nullptr;
		uint64_t m_DeviceAddress = 0;
	};
}

#endif