#pragma once
#ifndef FROSTIUM_OPENGL_IMPL

#include "Backends/Vulkan/VulkanDescriptor.h"

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	struct ComputePipelineCreateInfo;
	class  VulkanShader;

	class VulkanComputePipeline
	{
	public:
		void                                BeginCompute();
		void                                EndCompute();
		void                                Execute(uint32_t descriptorIndex, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
		void                                Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);

	private:
		bool                                Invalidate(ComputePipelineCreateInfo* pipelineSpec, VulkanShader* shader);

	private:
		VkDevice                            m_Device = nullptr;
		VkPipeline                          m_Pipeline = nullptr;
		VulkanShader*                       m_Shader = nullptr;
		VkPipelineCache                     m_PipelineCache = nullptr;
		VkDescriptorPool                    m_DescriptorPool = nullptr;
		VkPipelineLayout                    m_PipelineLayout = nullptr;
		ComputePipelineCreateInfo*          m_Spec = nullptr;
		std::vector<VulkanDescriptor>       m_Descriptors;
		std::vector<VkDescriptorSetLayout>  m_SetLayout;

		friend class ComputePipeline;
	};
}

#endif