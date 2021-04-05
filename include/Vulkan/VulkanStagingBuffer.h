#pragma once
#include "Vulkan/VulkanBuffer.h"

namespace Frostium
{
	class VulkanStagingBuffer: public VulkanBuffer
	{
	public:

		void Create(const void* data, uint64_t size);

		void Create(uint64_t size);
	};
}