#pragma once
#ifndef OPENGL_IMPL
#include "Backends/Vulkan/Vulkan.h"

#include <vector>
#include <unordered_map>

namespace SmolEngine
{
	class Shader;
	class Texture;
	class VulkanACStructure;
	enum class TextureFlags;
	struct BufferObject;

	class VulkanDescriptor
	{
	public:
		VulkanDescriptor();
		~VulkanDescriptor();

		void                                             Free();
		void                                             GenDescriptorSet(Ref<Shader>& shader, VkDescriptorPool pool);
		void                                             GenBuffersDescriptors(Ref<Shader>& shader);
		void                                             GenSamplersDescriptors(Ref<Shader>& shader);
		void                                             GenACStructureDescriptors(Ref<Shader>& shader, VulkanACStructure* baseStructure);
		bool                                             UpdateTextures(const std::vector<Ref<Texture>>& textures, uint32_t bindingPoint, TextureFlags usage);
		bool                                             UpdateTexture(const Ref<Texture>& texture, uint32_t bindingPoint, TextureFlags usage);
		bool                                             UpdateVkDescriptor(uint32_t bindingPoint, const VkDescriptorImageInfo& imageInfo, TextureFlags flags = TextureFlags::SAMPLER_2D);
		bool                                             UpdateVkAccelerationStructure(uint32_t bindingPoint, VulkanACStructure* structure);
		bool                                             UpdateBuffer(uint32_t binding, size_t size, const void* data, uint32_t offset = 0);
		VkDescriptorSet                                  GetDescriptorSets() const;
		static VkDescriptorType                          GetVkDescriptorType(TextureFlags flags);
		VkDescriptorSetLayout                            GetLayout() const;
														 
	private:											 
		VkWriteDescriptorSet                             CreateWriteSet(VkDescriptorSet descriptorSet, uint32_t binding, VkDescriptorBufferInfo* descriptorBufferInfo, 
			                                             VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		VkWriteDescriptorSet                             CreateWriteSet(VkDescriptorSet descriptorSet, uint32_t binding, VkDescriptorImageInfo* imageInfo, 
			                                             VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		VkWriteDescriptorSet                             CreateWriteSet(VkDescriptorSet descriptorSet, uint32_t binding, const std::vector<VkDescriptorImageInfo>& descriptorimageInfos,
			                                             VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	private:					   
		std::unordered_map<uint32_t, VkWriteDescriptorSet> m_WriteSets;
		std::unordered_map<uint32_t, Ref<BufferObject>> m_LocalBuffers;
		VkDescriptorSetLayout m_DescriptorSetLayout;
		VkDescriptorImageInfo m_ImageInfo;
		VkDescriptorSet m_DescriptorSet;
		VkDevice m_Device;

	private:

		friend class VulkanPipeline;
		friend class VulkanComputePipeline;
		friend class DeferredRenderer;
	};
}
#endif
