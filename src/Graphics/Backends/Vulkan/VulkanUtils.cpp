#include "stdafx.h"
#ifndef OPENGL_IMPL
#include "Graphics/Backends/Vulkan/VulkanUtils.h"
#include "Graphics/Backends/Vulkan/VulkanContext.h"

namespace SmolEngine
{
    void* VulkanUtils::AlignedAlloc(size_t size, size_t alignment)
    {
		void* data = nullptr;
#if defined(_MSC_VER) || defined(__MINGW32__)
		data = _aligned_malloc(size, alignment);
#else
		int res = posix_memalign(&data, alignment, size);
		if (res != 0)
			data = nullptr;
#endif
		return data;
    }

    void VulkanUtils::AlignedFree(void* data)
    {
#if	defined(_MSC_VER) || defined(__MINGW32__)
		_aligned_free(data);
#else
		free(data);
#endif
    }

	uint64_t VulkanUtils::GetBufferDeviceAddress(VkBuffer buffer)
	{
		auto& device = VulkanContext::GetDevice();

		VkBufferDeviceAddressInfoKHR bufferDeviceAI{};
		bufferDeviceAI.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		bufferDeviceAI.buffer = buffer;

		return device.vkGetBufferDeviceAddressKHR(device.GetLogicalDevice(), &bufferDeviceAI);
	}

	uint32_t VulkanUtils::GetAlignedSize(uint32_t value, uint32_t alignment)
	{
		return (value + alignment - 1) & ~(alignment - 1);
	}
}
#endif