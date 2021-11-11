#pragma once
#ifndef OPENGL_IMPL
#include "Graphics/Backends/Vulkan/VulkanACStructure.h"
#include "Graphics/Backends/Vulkan/VulkanDescriptor.h"

#include "Graphics/Primitives/RaytracingPipeline.h"

namespace SmolEngine
{
	struct BLAS
	{
		std::vector<Ref<VulkanACStructure>> Nodes{};
		uint32_t IntanceCount = 0;
	};

	class VulkanRaytracingPipeline: public RaytracingPipeline
	{
	public:
		struct ObjDesc
		{
			VkDeviceAddress vertexAddress = 0;
			VkDeviceAddress indexAddress = 0;
			VkDeviceAddress materialUIID = 0;
			VkDeviceAddress materialIndices = 0;
		};

		virtual void       SetCommandBuffer(void* cmdStorage = nullptr) override;
		virtual void       Dispatch(uint32_t width, uint32_t height) override;
		virtual void       Free() override;
		virtual bool       Build(RaytracingPipelineCreateInfo* info) override;
		virtual void       CreateScene(RaytracingPipelineSceneInfo* info) override;
		virtual void       UpdateScene(RaytracingPipelineSceneInfo* info) override;
		virtual void       SubmitPushConstant(ShaderType stage, size_t size, const void* data) override;
		virtual bool       UpdateBuffer(uint32_t binding, size_t size, const void* data, uint32_t offset = 0) override;
		virtual bool       UpdateTextures(const std::vector<Ref<Texture>>& textures, uint32_t bindingPoint, TextureFlags usage = TextureFlags::MAX_ENUM) override;
		virtual bool       UpdateTexture(const Ref<Texture>& texture, uint32_t bindingPoint, TextureFlags usage = TextureFlags::MAX_ENUM) override;
		virtual bool       UpdateTexture(const Ref<Framebuffer>& fb, uint32_t bindingPoint, uint32_t attachmentIndex = 0) override;
		virtual bool       UpdateTexture(const Ref<Framebuffer>& fb, uint32_t bindingPoint, const std::string& attachmentName) override;
		virtual bool       UpdateVkDescriptor(uint32_t bindingPoint, const void* descriptorPtr) override;
						   
		VkPipelineLayout   GetVkPipelineLayout() const;
		VkPipeline         GetVkPipeline() const;

	private:
		VkCommandBuffer                m_CommandBuffer = nullptr;
		VkPipelineLayout               m_PipelineLayout = nullptr;
		VkPipeline                     m_Pipeline = nullptr;
		VkDescriptorPool               m_DescriptorPool = nullptr;
		VulkanBuffer                   m_TransformBuffer{};
		VulkanBuffer                   m_InstanceBuffer{};
		std::vector<VulkanDescriptor>  m_Descriptors;
		std::vector<ObjDesc>           m_ObjDescriptions;
		std::map<Ref<Mesh>, Ref<BLAS>> m_BottomLevelAS;
		VulkanACStructure              m_TopLevelAS;
	};
}

#endif