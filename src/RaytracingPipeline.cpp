#include "stdafx.h"
#include "RaytracingPipeline.h"

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	PipelineCreateResult RaytracingPipeline::Create(const RaytracingPipelineCreateInfo* info)
	{
		return PipelineCreateResult();
	}
}