#pragma once
#ifndef OPENGL_IMPL
#include "Common/Memory.h"
#include "Common/DebugLog.h"
#include "Backends/Vulkan/VulkanAllocator.h"

#include <vulkan/include/vulkan/vulkan.h>

#define VK_CHECK_RESULT(f)															\
{																					\
	VkResult res = (f);																\
	if (res != VK_SUCCESS)															\
	{																				\
		DebugLog::LogError("VkResult is {}, in {} at line {}", res, __FILE__, __LINE__);  \
		assert(res == VK_SUCCESS);													\
	}																				\
}
#endif