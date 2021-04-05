#pragma once
#include "Vulkan/VulkanBuffer.h"

namespace Frostium
{
	class VulkanVertexBuffer: public VulkanBuffer
	{
	public:

		void Create(const void* data, uint64_t size, bool is_static = false);

		void Create(uint64_t size);

	};
}