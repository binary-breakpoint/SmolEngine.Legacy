#pragma once
#ifndef OPENGL_IMPL

#include "Backends/Vulkan/VulkanDescriptor.h"
#include "Backends/Vulkan/VulkanCommandBuffer.h"
#include "Primitives/ComputePipeline.h"

namespace SmolEngine
{
	class VulkanComputePipeline: public ComputePipeline
	{
	public:
		bool                           Build(ComputePipelineCreateInfo* info) override;
		bool                           Reload() override;
		void                           BeginCompute(void* cmdStorage = nullptr) override;
		void                           EndCompute() override;
		void                           Execute(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) override;
		void                           Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ, void* descriptorSet = nullptr) override;
		void                           SubmitPushConstant(size_t size, const void* data) override;
		virtual bool                   SubmitBuffer(uint32_t binding, size_t size, const void* data, uint32_t offset = 0) override;
		virtual bool                   UpdateSamplers(const std::vector<Ref<Texture>>& textures, uint32_t bindingPoint, bool storageImage = false) override;
		virtual bool                   UpdateSampler(Ref<Texture>& tetxure, uint32_t bindingPoint, bool storageImage = false) override;
		virtual bool                   UpdateSampler(Ref<Framebuffer>& framebuffer, uint32_t bindingPoint, uint32_t attachmentIndex = 0) override;
		virtual bool                   UpdateSampler(Ref<Framebuffer>& framebuffer, uint32_t bindingPoint, const std::string& attachmentName) override;
		virtual bool                   UpdateImageDescriptor(uint32_t bindingPoint, void* descriptor) override;
		virtual bool                   UpdateCubeMap(Ref<Texture>& cubeMap, uint32_t bindingPoint) override;
		VulkanDescriptor&              GeDescriptor(uint32_t index = 0);

	private:
		VkDevice                       m_Device = nullptr;
		VkPipeline                     m_Pipeline = nullptr;
		VkPipelineCache                m_PipelineCache = nullptr;
		VkDescriptorPool               m_DescriptorPool = nullptr;
		VkPipelineLayout               m_PipelineLayout = nullptr;
		CommandBufferStorage           m_CmdStorage{};
		std::vector<VulkanDescriptor>  m_Descriptors;
	};
}

#endif