#include "stdafx.h"

#ifdef OPENGL_IMPL
#else
#include "Graphics/Backends/Vulkan/VulkanComputePipeline.h"
#endif

namespace SmolEngine
{
	Ref<ComputePipeline> ComputePipeline::Create()
	{
		Ref<ComputePipeline> pipeline = nullptr;

#ifdef OPENGL_IMPL
#else
		pipeline = std::make_shared<VulkanComputePipeline>();
#endif
		return pipeline;
	}

	bool ComputePipeline::BuildBase(ComputePipelineCreateInfo* info)
	{
		if (info->ShaderPath.empty())
			return false;

		ShaderCreateInfo shaderCI = {};
		shaderCI.Buffers = info->ShaderBufferInfos;
		shaderCI.Stages[ShaderType::Compute] = info->ShaderPath;

		m_Info = *info;
		m_Shader = Shader::Create();
		m_Shader->Build(&shaderCI);

		return true;
	}
}