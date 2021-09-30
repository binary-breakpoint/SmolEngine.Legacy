#pragma once
#ifndef OPENGL_IMPL
#include "Backends/Vulkan/VulkanBuffer.h"

namespace SmolEngine
{
	class VulkanStagingBuffer : public VulkanBuffer
	{
	public:

		VulkanStagingBuffer() = default;
		~VulkanStagingBuffer() = default;

		void Create(const void* data, uint64_t size);
		void Create(uint64_t size);
	};
}
#endif