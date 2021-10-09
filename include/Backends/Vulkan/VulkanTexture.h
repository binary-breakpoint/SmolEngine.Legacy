#pragma once
#ifndef OPENGL_IMPL
#include "Backends/Vulkan/Vulkan.h"
#include "Primitives/Texture.h"

namespace SmolEngine
{
	class VulkanTexture: public Texture
	{
	public:
		~VulkanTexture();

		void                                       LoadFromFile(TextureCreateInfo* info) override;
		void                                       LoadFromMemory(const void* data, uint32_t size, TextureCreateInfo* info) override;
		void                                       LoadAsCubeFromKtx(TextureCreateInfo* info) override;
		void                                       LoadAsWhiteCube(TextureCreateInfo* info) override;
		void                                       LoadAsStorage(TextureCreateInfo* info) override;
		void                                       LoadAsWhite() override;
		void                                       Free() override;
		void                                       SetFormat(VkFormat format);

		uint32_t                                   GetMips() const override;
		std::pair<uint32_t, uint32_t>              GetMipSize(uint32_t mip) const override;
		VkDescriptorImageInfo                      GetMipImageView(uint32_t mip);
		const VkDescriptorImageInfo&               GetVkDescriptorImageInfo() const;
		VkImage                                    GetVkImage() const;
									               
		VkFormat                                   GetImageFormat(TextureFormat format);
		VkSamplerAddressMode                       GetVkSamplerAddressMode(AddressMode mode);
		VkBorderColor                              GetVkBorderColor(BorderColor color);
									               
		static void                                SetImageLayout(VkCommandBuffer cmdbuffer, VkImage image, VkImageLayout oldImageLayout, VkImageLayout newImageLayout,
			                                       VkImageSubresourceRange subresourceRange, VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
		static void                                SetImageLayout(VkCommandBuffer cmdbuffer, VkImage image, VkImageAspectFlags aspectMask, VkImageLayout oldImageLayout, VkImageLayout newImageLayout,
			                                       VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
		static VkImage                             CreateVkImage(uint32_t width, uint32_t height, int32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
			                                       VkDeviceMemory& imageMemory, uint32_t arrayLayers = 1);
		static void                                InsertImageMemoryBarrier(VkCommandBuffer cmdbuffer, VkImage image, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageLayout oldImageLayout, VkImageLayout newImageLayout,
			                                       VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkImageSubresourceRange subresourceRange);
	private:						               
		void                                       LoadEX(TextureCreateInfo* info, void* data, bool is_storage = false);
		void                                       GenerateMipMaps(VkImage image, VkCommandBuffer cmd, int32_t width, int32_t height, int32_t mipMaps, VkImageSubresourceRange& range);
		void                                       CreateSamplerAndImageView(uint32_t mipMaps, VkFormat format, bool anisotropy = true);
		void                                       FindTextureParams(TextureCreateInfo* info);

	private:

		VkDescriptorImageInfo                      m_DescriptorImageInfo;
		VkFormat                                   m_Format;
		VkImageLayout                              m_ImageLayout;
		VkFilter                                   m_Filter = VK_FILTER_LINEAR;
		VkSamplerAddressMode                       m_AddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		VkBorderColor                              m_BorderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		VkDevice                                   m_Device = nullptr;
		VkSampler                                  m_Samper = nullptr;
		VkImage                                    m_Image = nullptr;
		VkDeviceMemory                             m_DeviceMemory = nullptr;
		uint32_t                                   m_Mips = 0;
		VkImageView                                m_ImageView =  nullptr;
		std::unordered_map<uint32_t,VkImageView>   m_ImageViewMap;

	private:

		friend class VulkanPipeline;
		friend class VulkanPBR;
		friend class VulkanDescriptor;
	};
}
#endif