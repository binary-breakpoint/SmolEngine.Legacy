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

	bool RaytracingPipeline::BuildEX(RaytracingPipelineCreateInfo* info)
	{
		m_Shader = Shader::Create();
		m_Info = *info;

		ShaderCreateInfo shaderCI{};
		shaderCI.Buffers = info->Buffers;
		shaderCI.Stages[ShaderType::RayGen] = info->ShaderRayGenPath;
		shaderCI.Stages[ShaderType::RayMiss] = info->ShaderMissPath;
		shaderCI.Stages[ShaderType::RayCloseHit] = info->ShaderCloseHitPath;

		return m_Shader->Build(&shaderCI);
	}
}