#pragma once
#include "Vulkan/VulkanBuffer.h"

namespace Frostium
{
	class VulkanVertexBuffer: public VulkanBuffer
	{
	public:

		VulkanVertexBuffer() = default;

		~VulkanVertexBuffer() = default;

		void Create(const void* data, uint64_t size, bool is_static);

		void Create(uint64_t size);

	};
}