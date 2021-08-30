#pragma once
#include "Primitives/Shader.h"

#ifndef FROSTIUM_OPENGL_IMPL
#include "Backends/Vulkan/VulkanComputePipeline.h"
#endif

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	struct ComputePipelineCreateInfo
	{
		uint32_t                                         DescriptorCount = 1;
		std::string                                      ShaderPath = "";
		std::unordered_map<uint32_t, ShaderBufferInfo>   ShaderBufferInfos;
	};

#ifdef FROSTIUM_OPENGL_IMPL
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