#include "stdafx.h"
#include "Primitives/ComputePipeline.h"

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
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

#ifndef FROSTIUM_OPENGL_IMPL
		return Invalidate(info, m_Shader.GetVulkanShader());
#endif
	}

	bool ComputePipeline::Reload()
	{
		return false;
	}
}