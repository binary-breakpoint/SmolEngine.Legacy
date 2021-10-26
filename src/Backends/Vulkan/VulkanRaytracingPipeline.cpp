#include "stdafx.h"
#ifndef OPENGL_IMPL
#include "Backends/Vulkan/VulkanRaytracingPipeline.h"

namespace SmolEngine
{
	bool VulkanRaytracingPipeline::Build(RaytracingPipelineCreateInfo* info)
	{
		return false;
	}

	void VulkanRaytracingPipeline::SubmitPushConstant(ShaderType stage, size_t size, const void* data)
	{

	}
	bool VulkanRaytracingPipeline::SubmitBuffer(uint32_t binding, size_t size, const void* data, uint32_t offset)
	{
		return false;
	}

	bool VulkanRaytracingPipeline::UpdateSamplers(const std::vector<Ref<Texture>>& textures, uint32_t bindingPoint, bool storageImage)
	{
		return false;
	}

	bool VulkanRaytracingPipeline::UpdateSampler(Ref<Texture>& tetxure, uint32_t bindingPoint, bool storageImage)
	{
		return false;
	}

	bool VulkanRaytracingPipeline::UpdateSampler(Ref<Framebuffer>& framebuffer, uint32_t bindingPoint, uint32_t attachmentIndex)
	{
		return false;
	}

	bool VulkanRaytracingPipeline::UpdateSampler(Ref<Framebuffer>& framebuffer, uint32_t bindingPoint, const std::string& attachmentName)
	{
		return false;
	}

	bool VulkanRaytracingPipeline::UpdateImageDescriptor(uint32_t bindingPoint, void* descriptor)
	{
		return false;
	}

	bool VulkanRaytracingPipeline::UpdateCubeMap(Ref<Texture>& cubeMap, uint32_t bindingPoint)
	{
		return false;
	}
}


#endif