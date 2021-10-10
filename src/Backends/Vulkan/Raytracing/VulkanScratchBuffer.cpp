#include "stdafx.h"
#ifndef OPENGL_IMPL
#include "Backends/Vulkan/Raytracing/VulkanScratchBuffer.h"

namespace SmolEngine
{
	VulkanScratchBuffer::~VulkanScratchBuffer()
	{

	}

	void VulkanScratchBuffer::Create(size_t size)
	{

	}

	uint64_t VulkanScratchBuffer::GetDeviceAddress()
	{
		return m_DeviceAddress;
	}
}

#endif