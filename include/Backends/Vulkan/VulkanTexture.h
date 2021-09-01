#pragma once
#ifndef FROSTIUM_OPENGL_IMPL
#include "Backends/Vulkan/Vulkan.h"

#include "Common/Common.h"

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	enum class TextureFormat : int;
	enum class AddressMode : int;
	enum class BorderColor : int;
	struct TextureCreateInfo;
	struct TextureInfo;

	class VulkanTexture
	{
	public:
		VulkanTexture(TextureInfo* info);
		VulkanTexture();
		~VulkanTexture();

		void                            LoadTexture(const TextureCreateInfo* info);
		void                            LoadCubeMap(const TextureCreateInfo* info);
			                            
		void                            GenCubeMap(uint32_t width, uint32_t height, TextureFormat format);
		void                            GenTexture(uint32_t height, uint32_t width, const  TextureCreateInfo* info);
		void                            GenTexture(const void* data, uint32_t size, uint32_t width, uint32_t height, TextureFormat format);
		void                            GenWhiteTetxure(uint32_t width, uint32_t height);

		void                            Free();
		VkDescriptorImageInfo           GetMipImageView(uint32_t mip);
		void                            Resize(uint32_t width, uint32_t height);
		void                            SetFormat(VkFormat format);
		const VkDescriptorImageInfo&    GetVkDescriptorImageInfo() const;
		VkImage                         GetVkImage() const;
		uint32_t                        GetMips() const;
		std::pair<uint32_t, uint32_t>   GetMipSize(uint32_t mip) const;

		static void                     SetImageLayout(VkCommandBuffer cmdbuffer, VkImage image, VkImageLayout oldImageLayout, VkImageLayout newImageLayout,
			                            VkImageSubresourceRange subresourceRange, VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
		static void                     SetImageLayout(VkCommandBuffer cmdbuffer, VkImage image, VkImageAspectFlags aspectMask, VkImageLayout oldImageLayout, VkImageLayout newImageLayout,
			                            VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
		void                            CreateTexture(uint32_t width, uint32_t height, uint32_t mipMaps, const void* data, const TextureCreateInfo* info);
		static VkImage                  CreateVkImage(uint32_t width, uint32_t height, int32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
			                            VkDeviceMemory& imageMemory, uint32_t arrayLayers = 1);
		static void                     InsertImageMemoryBarrier(VkCommandBuffer cmdbuffer, VkImage image, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageLayout oldImageLayout, VkImageLayout newImageLayout,
			                            VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,VkImageSubresourceRange subresourceRange);
	private:
		void                            GenerateMipMaps(VkImage image, VkCommandBuffer cmd, int32_t width, int32_t height, int32_t mipMaps, VkImageSubresourceRange& range);
		void                            CreateFromBuffer(const void* data, VkDeviceSize size, uint32_t width, uint32_t height);
		void                            CreateSamplerAndImageView(uint32_t mipMaps, VkFormat format, bool anisotropy = true);

		VkFormat                        GetImageFormat(TextureFormat format);
		VkSamplerAddressMode            GetVkSamplerAddressMode(AddressMode mode);
		VkBorderColor                   GetVkBorderColor(BorderColor color);

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
		TextureInfo*                               m_Info = nullptr;
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