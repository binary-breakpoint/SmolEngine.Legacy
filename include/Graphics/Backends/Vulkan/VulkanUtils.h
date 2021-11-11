#pragma once
#ifndef OPENGL_IMPL

#include "Graphics/Backends/Vulkan/Vulkan.h"

namespace SmolEngine
{
	class VulkanUtils
	{
	public:
		// Wrapper functions for aligned memory allocation
		// There is currently no standard for this in C++ that works across all platforms and vendors, so we abstract this

		static void* AlignedAlloc(size_t size, size_t alignment);
		static void AlignedFree(void* data);

		static uint64_t GetBufferDeviceAddress(VkBuffer buffer);
		static uint32_t GetAlignedSize(uint32_t value, uint32_t alignment);
	};
}
#endif