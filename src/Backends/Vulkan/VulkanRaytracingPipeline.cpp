#include "stdafx.h"
#ifndef OPENGL_IMPL
#include "Backends/Vulkan/VulkanRaytracingPipeline.h"
#include "Backends/Vulkan/VulkanShader.h"
#include "Backends/Vulkan/VulkanPipeline.h"
#include "Backends/Vulkan/VulkanFramebuffer.h"

namespace SmolEngine
{
	bool VulkanRaytracingPipeline::Build(RaytracingPipelineCreateInfo* info)
	{
		if(!BuildEX(info)) { return false; }

		m_ACStructure.Build(info);

		VulkanPipeline::BuildDescriptors(m_Shader, info->NumDescriptorSets, m_Descriptors, m_DescriptorPool);


		return true;
	}

	void VulkanRaytracingPipeline::SubmitPushConstant(ShaderType stage, size_t size, const void* data)
	{
		vkCmdPushConstants(m_CommandBuffer, m_PipelineLayout, VulkanShader::GetVkShaderStage(stage), 0, static_cast<uint32_t>(size), data);
	}

	bool VulkanRaytracingPipeline::UpdateBuffer(uint32_t binding, size_t size, const void* data, uint32_t offset)
	{
		return m_Descriptors[m_DescriptorIndex].UpdateBuffer(binding, size, data, offset);
	}

	bool VulkanRaytracingPipeline::UpdateTextures(const std::vector<Ref<Texture>>& textures, uint32_t bindingPoint, TextureFlags usage)
	{
		return m_Descriptors[m_DescriptorIndex].UpdateTextures(textures, bindingPoint, usage);
	}

	bool VulkanRaytracingPipeline::UpdateTexture(const Ref<Texture>& texture, uint32_t bindingPoint, TextureFlags usage)
	{
		return m_Descriptors[m_DescriptorIndex].UpdateTexture(texture, bindingPoint, usage);
	}

	bool VulkanRaytracingPipeline::UpdateTexture(const Ref<Framebuffer>& fb, uint32_t bindingPoint, uint32_t attachmentIndex)
	{
		const auto& descriptor = fb->Cast<VulkanFramebuffer>()->GetAttachment(attachmentIndex)->ImageInfo;
		return m_Descriptors[m_DescriptorIndex].UpdateVkDescriptor(bindingPoint, descriptor);
	}

	bool VulkanRaytracingPipeline::UpdateTexture(const Ref<Framebuffer>& fb, uint32_t bindingPoint, const std::string& attachmentName)
	{
		const auto& descriptor = fb->Cast<VulkanFramebuffer>()->GetAttachment(attachmentName)->ImageInfo;
		return m_Descriptors[m_DescriptorIndex].UpdateVkDescriptor(bindingPoint, descriptor);
	}

	bool VulkanRaytracingPipeline::UpdateVkDescriptor(uint32_t bindingPoint, const void* descriptorPtr)
	{
		return m_Descriptors[m_DescriptorIndex].UpdateVkDescriptor(bindingPoint, *(VkDescriptorImageInfo*)descriptorPtr);
	}

	VkPipelineLayout VulkanRaytracingPipeline::GetVkPipelineLayout() const
	{
		return m_PipelineLayout;
	}

	VkPipeline VulkanRaytracingPipeline::GetVkPipeline() const
	{
		return m_Pipeline;
	}
}

#endif