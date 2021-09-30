#include "stdafx.h"
#include "Primitives/ComputePipeline.h"
#include "Backends/Vulkan/VulkanPipeline.h"
#include "Backends/Vulkan/VulkanShader.h"

#ifndef OPENGL_IMPL

namespace SmolEngine
{
	bool VulkanComputePipeline::Invalidate(ComputePipelineCreateInfo* pipelineSpec, VulkanShader* shader)
	{
		VulkanPipeline::BuildDescriptors(shader, pipelineSpec->DescriptorCount, m_Descriptors, m_DescriptorPool);

		m_Spec = pipelineSpec;
		m_Shader = shader;
		m_Device = VulkanContext::GetDevice().GetLogicalDevice();

		VkPipelineLayoutCreateInfo pipelineLayoutCI = {};
		{
			pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutCI.pNext = nullptr;
			auto layout = m_Descriptors[0].GetLayout();
			pipelineLayoutCI.pSetLayouts = &layout;
			pipelineLayoutCI.setLayoutCount = 1;
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

	void VulkanComputePipeline::BeginCompute(CommandBufferStorage* cmdStorage)
	{
		if (!cmdStorage)
		{
			m_CmdStorage = {};
			m_CmdStorage.bNewPool = false;
			m_CmdStorage.bCompute = true;
			m_CmdStorage.bStartRecord = true;
			VulkanCommandBuffer::CreateCommandBuffer(&m_CmdStorage);
		}
		else
		{
			m_CmdStorage = *cmdStorage;
		}

		vkCmdBindPipeline(m_CmdStorage.Buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pipeline);
	}

	void VulkanComputePipeline::BeginCompute(VkCommandBuffer cmdBuffer)
	{
		m_CmdStorage.Buffer = cmdBuffer;
		vkCmdBindPipeline(m_CmdStorage.Buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pipeline);
	}

	void VulkanComputePipeline::EndCompute()
	{
		VulkanCommandBuffer::ExecuteCommandBuffer(&m_CmdStorage);
	}

	void VulkanComputePipeline::Execute(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
	{
		for (uint32_t i = 0; i < static_cast<uint32_t>(m_Descriptors.size()); ++i)
		{
			Dispatch(groupCountX, groupCountY, groupCountZ, i);
		}

		VulkanCommandBuffer::ExecuteCommandBuffer(&m_CmdStorage);
	}

	void VulkanComputePipeline::Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ, uint32_t descriptorIndex, VkDescriptorSet descriptorSet)
	{
		const auto& set = descriptorSet == nullptr ? m_Descriptors[descriptorIndex].GetDescriptorSets() : descriptorSet;
		vkCmdBindDescriptorSets(m_CmdStorage.Buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_PipelineLayout, 0, 1, &set, 0, 0);
		vkCmdDispatch(m_CmdStorage.Buffer, groupCountX, groupCountY, groupCountZ);
	}

	void VulkanComputePipeline::SubmitPushConstant(size_t size, const void* data)
	{
		vkCmdPushConstants(m_CmdStorage.Buffer, m_PipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, static_cast<uint32_t>(size), data);
	}

	VulkanDescriptor& VulkanComputePipeline::GeDescriptor(uint32_t index)
	{
		return m_Descriptors[index];
	}
}

#endif