#include "stdafx.h"
#include "Primitives/ComputePipeline.h"

namespace SmolEngine
{
	bool ComputePipeline::Create(ComputePipelineCreateInfo* info)
	{
		if(info->ShaderPath.empty())
			return false;

		ShaderCreateInfo shaderCI = {};
		shaderCI.BufferInfos = info->ShaderBufferInfos;
		shaderCI.FilePaths[ShaderType::Compute] = info->ShaderPath;

		m_Shader = {};
		Shader::Create(&shaderCI, &m_Shader);

#ifndef OPENGL_IMPL
		return Invalidate(info, m_Shader.GetVulkanShader());
#endif
	}

	bool ComputePipeline::Reload()
	{
		return false;
	}
}