#include "stdafx.h"

#ifndef OPENGL_IMPL
#include "Backends/Vulkan/VulkanComputePipeline.h"
#include "Backends/Vulkan/VulkanContext.h"
#include "Backends/Vulkan/VulkanPipeline.h"
#include "Backends/Vulkan/VulkanShader.h"

#include "Primitives/ComputePipeline.h"

namespace SmolEngine
{
	void VulkanComputePipeline::BeginCompute(void* cmdStorage)
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
			m_CmdStorage = *static_cast<CommandBufferStorage*>(cmdStorage);

		vkCmdBindPipeline(m_CmdStorage.Buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pipeline);
	}

	bool VulkanComputePipeline::Build(ComputePipelineCreateInfo* info)
	{
		if (BuildBase(info))
		{
			m_Device = VulkanContext::GetDevice().GetLogicalDevice();
			VulkanShader* shader = m_Shader->Cast<VulkanShader>();

			VulkanPipeline::BuildDescriptors(m_Shader, info->DescriptorCount, m_Descriptors, m_DescriptorPool);

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

			auto& stages = shader->GetVkPipelineShaderStages();
			computePipelineCreateInfo.stage = stages[0];

			VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
			pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

			VK_CHECK_RESULT(vkCreatePipelineCache(m_Device, &pipelineCacheCreateInfo, nullptr, &m_PipelineCache));
			VK_CHECK_RESULT(vkCreateComputePipelines(m_Device, m_PipelineCache, 1, &computePipelineCreateInfo, nullptr, &m_Pipeline));

			return true;
		}

		return false;
	}

	bool VulkanComputePipeline::Reload()
	{
		return false;
	}

	void VulkanComputePipeline::EndCompute()
	{
		VulkanCommandBuffer::ExecuteCommandBuffer(&m_CmdStorage);
	}

	void VulkanComputePipeline::Execute(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
	{
		for (uint32_t i = 0; i < static_cast<uint32_t>(m_Descriptors.size()); ++i)
		{
			SetDescriptorIndex(i);
			Dispatch(groupCountX, groupCountY, groupCountZ);
		}

		SetDescriptorIndex(0);
		VulkanCommandBuffer::ExecuteCommandBuffer(&m_CmdStorage);
	}

	void VulkanComputePipeline::Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ, void* descriptorSet)
	{
		const auto& set = m_Descriptors[m_DescriptorIndex].GetDescriptorSets();
		vkCmdBindDescriptorSets(m_CmdStorage.Buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_PipelineLayout, 0, 1, &set, 0, 0);
		vkCmdDispatch(m_CmdStorage.Buffer, groupCountX, groupCountY, groupCountZ);
	}

	void VulkanComputePipeline::SubmitPushConstant(size_t size, const void* data)
	{
		vkCmdPushConstants(m_CmdStorage.Buffer, m_PipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, static_cast<uint32_t>(size), data);
	}

	bool VulkanComputePipeline::UpdateBuffer(uint32_t binding, size_t size, const void* data, uint32_t offset)
	{
		return m_Descriptors[m_DescriptorIndex].UpdateBuffer(binding, size, data, offset);
	}

	bool VulkanComputePipeline::UpdateTextures(const std::vector<Ref<Texture>>& textures, uint32_t bindingPoint, TextureFlags usage)
	{
		return m_Descriptors[m_DescriptorIndex].UpdateTextures(textures, bindingPoint, usage);
	}

	bool VulkanComputePipeline::UpdateTexture(const Ref<Texture>& texture, uint32_t bindingPoint, TextureFlags usage)
	{
		return m_Descriptors[m_DescriptorIndex].UpdateTexture(texture, bindingPoint, usage);
	}

	bool VulkanComputePipeline::UpdateTexture(const Ref<Framebuffer>& fb, uint32_t bindingPoint, uint32_t attachmentIndex)
	{
		const auto& descriptor = fb->Cast<VulkanFramebuffer>()->GetAttachment(attachmentIndex)->ImageInfo;
		return m_Descriptors[m_DescriptorIndex].UpdateVkDescriptor(bindingPoint, descriptor);
	}

	bool VulkanComputePipeline::UpdateTexture(const Ref<Framebuffer>& fb, uint32_t bindingPoint, const std::string& attachmentName)
	{
		const auto& descriptor = fb->Cast<VulkanFramebuffer>()->GetAttachment(attachmentName)->ImageInfo;
		return m_Descriptors[m_DescriptorIndex].UpdateVkDescriptor(bindingPoint, descriptor);
	}

	bool VulkanComputePipeline::UpdateVkDescriptor(uint32_t bindingPoint, const void* descriptorPtr)
	{
		return m_Descriptors[m_DescriptorIndex].UpdateVkDescriptor(bindingPoint, *(VkDescriptorImageInfo*)descriptorPtr);
	}

	VulkanDescriptor& VulkanComputePipeline::GeDescriptor(uint32_t index)
	{
		return m_Descriptors[index];
	}
}

#endif