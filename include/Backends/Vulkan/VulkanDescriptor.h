#pragma once
#ifndef OPENGL_IMPL
#include "Backends/Vulkan/Vulkan.h"

#include <vector>
#include <unordered_map>

namespace SmolEngine
{
	class Shader;
	class Texture;
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
			                                             
		bool                                             Update2DSamplers(const std::vector<Ref<Texture>>& textures, uint32_t bindingPoint, bool storage = false);
		bool                                             UpdateImageResource(uint32_t bindingPoint, const VkDescriptorImageInfo& imageInfo, bool storage = false);
		bool                                             UpdateCubeMap(Ref<Texture>& cubeMap, uint32_t bindingPoint);
		bool                                             UpdateBuffer(uint32_t binding, size_t size, const void* data, uint32_t offset = 0);
		void                                             UpdateWriteSets();
														 
		VkDescriptorSet                                  GetDescriptorSets() const;
		VkDescriptorSetLayout                            GetLayout() const;
														 
	private:											 
		VkWriteDescriptorSet                             CreateWriteSet(VkDescriptorSet descriptorSet, uint32_t binding, VkDescriptorBufferInfo* descriptorBufferInfo, 
			                                             VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		VkWriteDescriptorSet                             CreateWriteSet(VkDescriptorSet descriptorSet, uint32_t binding, VkDescriptorImageInfo* imageInfo, 
			                                             VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		VkWriteDescriptorSet                             CreateWriteSet(VkDescriptorSet descriptorSet, uint32_t binding, const std::vector<VkDescriptorImageInfo>& descriptorimageInfos,
			                                             VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	private:
												    
		VkDescriptorSetLayout                            m_DescriptorSetLayout;
		VkDescriptorSet                                  m_DescriptorSet;
		VkDevice                                         m_Device;
												         
		std::vector<VkWriteDescriptorSet>                m_WriteSets;
		std::unordered_map<uint32_t, Ref<BufferObject>>  m_LocalBuffers;
		VkDescriptorImageInfo                            m_ImageInfo;

	private:

		friend class VulkanPipeline;
		friend class VulkanComputePipeline;
		friend class DeferredRenderer;
	};
}
#endif
