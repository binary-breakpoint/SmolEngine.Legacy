#include "stdafx.h"
#include "Backends/Vulkan/VulkanComputePipeline.h"
#include "Backends/Vulkan/VulkanPipeline.h"
#include "Backends/Vulkan/VulkanShader.h"

#ifndef FROSTIUM_OPENGL_IMPL

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	bool VulkanComputePipeline::Invalidate(ComputePipelineCreateInfo* pipelineSpec, VulkanShader* shader)
	{
		VulkanPipeline::BuildDescriptors(shader, 1, m_Descriptors, m_DescriptorPool);

		m_Spec = pipelineSpec;
		m_Shader = shader;
		m_Device = VulkanContext::GetDevice().GetLogicalDevice();
		m_SetLayout.clear();

		for (auto& descriptor : m_Descriptors)
			m_SetLayout.push_back(descriptor.m_DescriptorSetLayout);

		VkPipelineLayoutCreateInfo pipelineLayoutCI = {};
		{
			pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutCI.pNext = nullptr;
			pipelineLayoutCI.setLayoutCount = static_cast<uint32_t>(m_SetLayout.size());
			pipelineLayoutCI.pSetLayouts = m_SetLayout.data();
			pipelineLayoutCI.pushConstantRangeCount = static_cast<uint32_t>(shader->m_VkPushConstantRanges.size());
			pipelineLayoutCI.pPushConstantRanges = shader->m_VkPushConstantRanges.data();

			VK_CHECK_RESULT(vkCreatePipelineLayout(m_Device, &pipelineLayoutCI, nullptr, &m_PipelineLayout));
		}

		VkComputePipelineCreateInfo computePipelineCreateInfo{};
		computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		computePipelineCreateInfo.layout = m_PipelineLayout;
		computePipelineCreateInfo.flags = 0;

		auto& stages = m_Shader->GetVkPipelineShaderStages();
		computePipelineCreateInfo.stage = stages[0];

		VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
		pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

		VK_CHECK_RESULT(vkCreatePipelineCache(m_Device, &pipelineCacheCreateInfo, nullptr, &m_PipelineCache));
		VK_CHECK_RESULT(vkCreateComputePipelines(m_Device, m_PipelineCache, 1, &computePipelineCreateInfo, nullptr, &m_Pipeline));

		return true;
	}

	void VulkanComputePipeline::BeginCompute()
	{

	}

	void VulkanComputePipeline::EndCompute()
	{

	}

	void VulkanComputePipeline::Execute(uint32_t descriptorIndex, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
	{

	}

	void VulkanComputePipeline::Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
	{

	}
}

#endif