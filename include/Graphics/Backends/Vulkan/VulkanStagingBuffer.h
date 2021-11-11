#pragma once
#ifndef OPENGL_IMPL
#include "Graphics/Backends/Vulkan/VulkanBuffer.h"

namespace SmolEngine
{
	class VulkanStagingBuffer : public VulkanBuffer
	{
	public:
		void Create(const void* data, uint64_t size);
		void Create(uint64_t size);
	};
}
#endif