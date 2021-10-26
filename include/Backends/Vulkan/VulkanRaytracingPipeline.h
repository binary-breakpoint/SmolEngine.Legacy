#pragma once
#ifndef OPENGL_IMPL

#include "Primitives/RaytracingPipeline.h"
#include "Backends/Vulkan/Vulkan.h"

namespace SmolEngine
{
	class VulkanRaytracingPipeline: public RaytracingPipeline
	{
	public:
		virtual bool  Build(RaytracingPipelineCreateInfo* info) override;
		virtual void  SubmitPushConstant(ShaderType stage, size_t size, const void* data) override;

		virtual bool  SubmitBuffer(uint32_t binding, size_t size, const void* data, uint32_t offset = 0) override;
		virtual bool  UpdateSamplers(const std::vector<Ref<Texture>>& textures, uint32_t bindingPoint, bool storageImage = false) override;
		virtual bool  UpdateSampler(Ref<Texture>& tetxure, uint32_t bindingPoint, bool storageImage = false) override;
		virtual bool  UpdateSampler(Ref<Framebuffer>& framebuffer, uint32_t bindingPoint, uint32_t attachmentIndex = 0) override;
		virtual bool  UpdateSampler(Ref<Framebuffer>& framebuffer, uint32_t bindingPoint, const std::string& attachmentName) override;
		virtual bool  UpdateImageDescriptor(uint32_t bindingPoint, void* descriptor) override;
		virtual bool  UpdateCubeMap(Ref<Texture>& cubeMap, uint32_t bindingPoint) override;
	};
}

#endif