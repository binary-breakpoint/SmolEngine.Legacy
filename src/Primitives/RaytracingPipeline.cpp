#include "stdafx.h"
#include "Primitives/RaytracingPipeline.h"

#ifndef OPENGL_IMPL
#include "Backends/Vulkan/VulkanRaytracingPipeline.h"
#endif

namespace SmolEngine
{
	Ref<RaytracingPipeline> RaytracingPipeline::Create()
	{
		Ref<RaytracingPipeline> pipeline = nullptr;
#ifndef OPENGL_IMPL
		pipeline = std::make_shared<VulkanRaytracingPipeline>();
#else
#endif
		return pipeline;
	}
}