#pragma once
#ifndef OPENGL_IMPL
#include "Backends/Vulkan/VulkanACStructure.h"
#include "Backends/Vulkan/VulkanDescriptor.h"

#include "Primitives/RaytracingPipeline.h"

namespace SmolEngine
{
	class VulkanRaytracingPipeline: public RaytracingPipeline
	{
	public:
		virtual void     Dispatch(uint32_t width, uint32_t height, void* cmdStorage = nullptr) override;
		virtual void     Free() override;
		virtual bool     Build(RaytracingPipelineCreateInfo* info) override;
		virtual void     CreateScene(RaytracingPipelineSceneInfo* info) override;
		virtual void     UpdateScene(RaytracingPipelineSceneInfo* info) override;
		virtual void     SubmitPushConstant(ShaderType stage, size_t size, const void* data) override;
		virtual bool     UpdateBuffer(uint32_t binding, size_t size, const void* data, uint32_t offset = 0) override;
		virtual bool     UpdateTextures(const std::vector<Ref<Texture>>& textures, uint32_t bindingPoint, TextureFlags usage = TextureFlags::MAX_ENUM) override;
		virtual bool     UpdateTexture(const Ref<Texture>& texture, uint32_t bindingPoint, TextureFlags usage = TextureFlags::MAX_ENUM) override;
		virtual bool     UpdateTexture(const Ref<Framebuffer>& fb, uint32_t bindingPoint, uint32_t attachmentIndex = 0) override;
		virtual bool     UpdateTexture(const Ref<Framebuffer>& fb, uint32_t bindingPoint, const std::string& attachmentName) override;
		virtual bool     UpdateVkDescriptor(uint32_t bindingPoint, const void* descriptorPtr) override;

		VkPipelineLayout GetVkPipelineLayout() const;
		VkPipeline       GetVkPipeline() const;

	private:
		VkCommandBuffer                    m_CommandBuffer = nullptr;
		VkPipelineLayout                   m_PipelineLayout = nullptr;
		VkPipeline                         m_Pipeline = nullptr;
		VkDescriptorPool                   m_DescriptorPool = nullptr;
		VulkanACStructure                  m_ACStructure{};
		std::vector<VulkanDescriptor>      m_Descriptors;
	};
}

#endif