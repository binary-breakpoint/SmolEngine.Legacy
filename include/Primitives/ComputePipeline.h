#pragma once
#include "Primitives/Shader.h"

#ifndef OPENGL_IMPL
#include "Backends/Vulkan/VulkanComputePipeline.h"
#endif

namespace SmolEngine
{
	struct ComputePipelineCreateInfo
	{
		uint32_t                                         DescriptorCount = 1;
		std::string                                      ShaderPath = "";
		std::unordered_map<uint32_t, ShaderBufferInfo>   ShaderBufferInfos;
	};

#ifdef OPENGL_IMPL
	class ComputePipeline
#else
	class ComputePipeline: public VulkanComputePipeline
#endif
	{
	public:
		bool   Create(ComputePipelineCreateInfo* info);
		bool   Reload();

	private:
		Shader m_Shader;
	};
}