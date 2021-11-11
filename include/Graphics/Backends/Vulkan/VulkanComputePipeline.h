#pragma once
#ifndef OPENGL_IMPL

#include "Graphics/Backends/Vulkan/VulkanDescriptor.h"
#include "Graphics/Backends/Vulkan/VulkanCommandBuffer.h"
#include "Graphics/Primitives/ComputePipeline.h"

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
		bool                           UpdateBuffer(uint32_t binding, size_t size, const void* data, uint32_t offset = 0) override;
		bool                           UpdateTextures(const std::vector<Ref<Texture>>& textures, uint32_t bindingPoint, TextureFlags usage = TextureFlags::MAX_ENUM) override;
		bool                           UpdateTexture(const Ref<Texture>& texture, uint32_t bindingPoint, TextureFlags usage = TextureFlags::MAX_ENUM) override;
		bool                           UpdateTexture(const Ref<Framebuffer>& fb, uint32_t bindingPoint, uint32_t attachmentIndex = 0) override;
		bool                           UpdateTexture(const Ref<Framebuffer>& fb, uint32_t bindingPoint, const std::string& attachmentName) override;
		bool                           UpdateVkDescriptor(uint32_t bindingPoint, const void* descriptorPtr) override;
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