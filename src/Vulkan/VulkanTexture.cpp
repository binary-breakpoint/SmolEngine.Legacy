#include "stdafx.h"
#ifndef FROSTIUM_OPENGL_IMPL
#include "Primitives/Texture.h"
#include "Vulkan/VulkanContext.h"
#include "Vulkan/VulkanStagingBuffer.h"
#include "Vulkan/VulkanMemoryAllocator.h"

#include <imgui/examples/imgui_impl_vulkan.h>

#include <ktx.h>
#include <ktxvulkan.h>
#include <stb_image.h>

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	VulkanTexture::VulkanTexture(TextureInfo* info)
		: m_Info(info), 
		  m_Device(VulkanContext::GetDevice().GetLogicalDevice())
	{

	}

	VulkanTexture::~VulkanTexture()
	{
		if (m_Device)
		{
			if(m_Image != VK_NULL_HANDLE)
				vkDestroyImage(m_Device, m_Image, nullptr);

			if(m_ImageView != VK_NULL_HANDLE)
				vkDestroyImageView(m_Device, m_ImageView, nullptr);

			if(m_Samper != VK_NULL_HANDLE)
				vkDestroySampler(m_Device, m_Samper, nullptr);

			if(m_DeviceMemory != VK_NULL_HANDLE)
				vkFreeMemory(m_Device, m_DeviceMemory, nullptr);
		}
	}

	void VulkanTexture::GenTexture(const void* data, uint32_t size, uint32_t width, uint32_t height, TextureFormat format)
	{
		m_Format = GetImageFormat(format);

		CreateFromBuffer(data, size, width, height);
	}

	void VulkanTexture::GenWhiteTetxure(uint32_t width, uint32_t height)
	{
		uint32_t whiteTextureData = 0xffffffff;
		TextureCreateInfo info = {};
		
		m_Format = GetImageFormat(info.eFormat);
		m_Info->Width = width;
		m_Info->Height = height;

		CreateTexture(width, height, 1, &whiteTextureData, &info);
	}

	void VulkanTexture::LoadTexture(const TextureCreateInfo* info)
	{
		int height, width, channels;
		if(info->bVerticalFlip)
			stbi_set_flip_vertically_on_load(1);

		stbi_uc* data = nullptr;
		{
			data = stbi_load(info->FilePath.c_str(), &width, &height, &channels, 4);
			if (!data)
			{
				NATIVE_ERROR("VulkanTexture:: Texture not found! file: {}, line: {}", __FILE__, __LINE__);
				abort();
			}
		}

		const uint32_t mipLevels = static_cast<uint32_t>(floor(log2(std::max(width, height)))) + 1;

		m_Format = GetImageFormat(info->eFormat);
		m_Info->Width = width;
		m_Info->Height = height;
		m_Filter = info->eFilter == ImageFilter::LINEAR ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
		m_AddressMode = GetVkSamplerAddressMode(info->eAddressMode);
		m_BorderColor = GetVkBorderColor(info->eBorderColor);

		CreateTexture(width, height, mipLevels, data, info);
		stbi_image_free(data);
	}

	void VulkanTexture::GenCubeMap(uint32_t width, uint32_t height, TextureFormat format)
	{
		m_Format = GetImageFormat(format);
		m_Info->Width  = width;
		m_Info->Height = height;

		// Create optimal tiled target image
		VkImageCreateInfo imageCreateInfo = {};
		{
			imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
			imageCreateInfo.format = m_Format;
			imageCreateInfo.mipLevels = 1;
			imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageCreateInfo.extent = { width, height, 1 };
			imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			// Cube faces count as array layers in Vulkan
			imageCreateInfo.arrayLayers = 6;
			// This flag is required for cube map images
			imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

			VK_CHECK_RESULT(vkCreateImage(m_Device, &imageCreateInfo, nullptr, &m_Image));
		}

		VkMemoryRequirements memReqs;
		vkGetImageMemoryRequirements(m_Device, m_Image, &memReqs);
		uint32_t typeIndex = VulkanContext::GetDevice().GetMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VulkanMemoryAllocator::Allocate(m_Device, memReqs, &m_DeviceMemory, typeIndex);
		VK_CHECK_RESULT(vkBindImageMemory(m_Device, m_Image, m_DeviceMemory, 0));
		// View
		{
			VkImageViewCreateInfo view = {};
			view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			// Cube map view type
			view.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
			view.format = m_Format;
			view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
			view.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
			// 6 array layers (faces)
			view.subresourceRange.layerCount = 6;
			// Set number of mip levels
			view.subresourceRange.levelCount = 1;
			view.image = m_Image;
			VK_CHECK_RESULT(vkCreateImageView(m_Device, &view, nullptr, &m_ImageView));
		}

		// Image barrier for optimal image (target)
		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = 1;
		subresourceRange.layerCount = 6;

		CommandBufferStorage cmdStorage{};
		VulkanCommandBuffer::CreateCommandBuffer(&cmdStorage);
		{
			SetImageLayout(
				cmdStorage.Buffer,
				m_Image,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				subresourceRange);
		}
		VulkanCommandBuffer::ExecuteCommandBuffer(&cmdStorage);

		// Sampler
		{
			auto& device = VulkanContext::GetDevice();

			// Create sampler
			VkSamplerCreateInfo samplerCI = {};
			samplerCI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerCI.maxAnisotropy = 1.0f;
			samplerCI.magFilter = VK_FILTER_LINEAR;
			samplerCI.minFilter = VK_FILTER_LINEAR;
			samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerCI.addressModeV = samplerCI.addressModeU;
			samplerCI.addressModeW = samplerCI.addressModeU;
			samplerCI.mipLodBias = 0.0f;
			samplerCI.compareOp = VK_COMPARE_OP_NEVER;
			samplerCI.minLod = 0.0f;
			samplerCI.maxLod = 1.0f;
			samplerCI.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
			samplerCI.maxAnisotropy = 1.0f;
			if (device.GetDeviceFeatures()->samplerAnisotropy)
			{
				samplerCI.maxAnisotropy = device.GetDeviceProperties()->limits.maxSamplerAnisotropy;
				samplerCI.anisotropyEnable = VK_TRUE;
			}

			VK_CHECK_RESULT(vkCreateSampler(m_Device, &samplerCI, nullptr, &m_Samper));
		}

		m_ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		m_DescriptorImageInfo = {};
		m_DescriptorImageInfo.imageLayout = m_ImageLayout;
		m_DescriptorImageInfo.imageView = m_ImageView;
		m_DescriptorImageInfo.sampler = m_Samper;
	}

	void VulkanTexture::LoadCubeMap(const TextureCreateInfo* info)
	{
		ktxResult result;
		ktxTexture* ktxTexture;
		result = ktxTexture_CreateFromNamedFile(info->FilePath.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);
		assert(result == KTX_SUCCESS);

		uint32_t width = ktxTexture->baseWidth;
		uint32_t height = ktxTexture->baseHeight;
		uint32_t mipLevels = ktxTexture->numLevels;

		ktx_uint8_t* ktxTextureData = ktxTexture_GetData(ktxTexture);
		ktx_size_t ktxTextureSize = ktxTexture_GetSize(ktxTexture);

		m_Format = GetImageFormat(info->eFormat);
		m_Info->Width = width;
		m_Info->Height  = height;
		m_Filter = info->eFilter == ImageFilter::LINEAR ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
		m_AddressMode = GetVkSamplerAddressMode(info->eAddressMode);
		m_BorderColor = GetVkBorderColor(info->eBorderColor);

		VulkanBuffer stagingBuffer;
		stagingBuffer.CreateBuffer(ktxTextureSize, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
		stagingBuffer.SetData(ktxTextureData, ktxTextureSize);

		// Create optimal tiled target image
		VkImageCreateInfo imageCreateInfo = {};
		{
			imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
			imageCreateInfo.format = m_Format;
			imageCreateInfo.mipLevels = mipLevels;
			imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageCreateInfo.extent = { width, height, 1 };
			imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			// Cube faces count as array layers in Vulkan
			imageCreateInfo.arrayLayers = 6;
			// This flag is required for cube map images
			imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

			VK_CHECK_RESULT(vkCreateImage(m_Device, &imageCreateInfo, nullptr, &m_Image));
		}

		VkMemoryRequirements memReqs;
		vkGetImageMemoryRequirements(m_Device, m_Image, &memReqs);
		uint32_t typeIndex = VulkanContext::GetDevice().GetMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VulkanMemoryAllocator::Allocate(m_Device, memReqs, &m_DeviceMemory, typeIndex);
		VK_CHECK_RESULT(vkBindImageMemory(m_Device, m_Image, m_DeviceMemory, 0));

		// Setup buffer copy regions for each face including all of its miplevels
		std::vector<VkBufferImageCopy> bufferCopyRegions;
		uint32_t offset = 0;
		for (uint32_t face = 0; face < 6; face++)
		{
			for (uint32_t level = 0; level < mipLevels; level++)
			{
				// Calculate offset into staging buffer for the current mip level and face
				ktx_size_t offset;
				KTX_error_code ret = ktxTexture_GetImageOffset(ktxTexture, level, 0, face, &offset);
				assert(ret == KTX_SUCCESS);
				VkBufferImageCopy bufferCopyRegion = {};
				bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				bufferCopyRegion.imageSubresource.mipLevel = level;
				bufferCopyRegion.imageSubresource.baseArrayLayer = face;
				bufferCopyRegion.imageSubresource.layerCount = 1;
				bufferCopyRegion.imageExtent.width = ktxTexture->baseWidth >> level;
				bufferCopyRegion.imageExtent.height = ktxTexture->baseHeight >> level;
				bufferCopyRegion.imageExtent.depth = 1;
				bufferCopyRegion.bufferOffset = offset;
				bufferCopyRegions.push_back(bufferCopyRegion);
			}
		}

		// Image barrier for optimal image (target)
		// Set initial layout for all array layers (faces) of the optimal (target) tiled texture
		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = mipLevels;
		subresourceRange.layerCount = 6;

		m_ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		CommandBufferStorage cmdStorage{};
		VulkanCommandBuffer::CreateCommandBuffer(&cmdStorage);
		{
			SetImageLayout(
				cmdStorage.Buffer,
				m_Image,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				subresourceRange);

			vkCmdCopyBufferToImage(
				cmdStorage.Buffer,
				stagingBuffer.GetBuffer(),
				m_Image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				static_cast<uint32_t>(bufferCopyRegions.size()),
				bufferCopyRegions.data()
			);

			SetImageLayout(
				cmdStorage.Buffer,
				m_Image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				m_ImageLayout,
				subresourceRange);
		}
		VulkanCommandBuffer::ExecuteCommandBuffer(&cmdStorage);

		// Sampler
		{
			auto& device = VulkanContext::GetDevice();

			// Create sampler
			VkSamplerCreateInfo samplerCI = {};
			samplerCI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerCI.maxAnisotropy = 1.0f;
			samplerCI.magFilter = m_Filter;
			samplerCI.minFilter = m_Filter;
			samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerCI.addressModeU = m_AddressMode;
			samplerCI.addressModeV = samplerCI.addressModeU;
			samplerCI.addressModeW = samplerCI.addressModeU;
			samplerCI.mipLodBias = 0.0f;
			samplerCI.compareOp = VK_COMPARE_OP_NEVER;
			samplerCI.minLod = 0.0f;
			samplerCI.maxLod = static_cast<float>(mipLevels);
			samplerCI.borderColor = m_BorderColor;
			samplerCI.maxAnisotropy = 1.0f;
			if (device.GetDeviceFeatures()->samplerAnisotropy && info->bAnisotropyEnable)
			{
				samplerCI.maxAnisotropy = device.GetDeviceProperties()->limits.maxSamplerAnisotropy;
				samplerCI.anisotropyEnable = VK_TRUE;
			}

			VK_CHECK_RESULT(vkCreateSampler(m_Device, &samplerCI, nullptr, &m_Samper));
		}

		// View
		{
			VkImageViewCreateInfo view = {};
			view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			// Cube map view type
			view.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
			view.format = m_Format;
			view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
			view.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
			// 6 array layers (faces)
			view.subresourceRange.layerCount = 6;
			// Set number of mip levels
			view.subresourceRange.levelCount = mipLevels;
			view.image = m_Image;
			VK_CHECK_RESULT(vkCreateImageView(m_Device, &view, nullptr, &m_ImageView));
		}

		m_DescriptorImageInfo = {};
		m_DescriptorImageInfo.imageLayout = m_ImageLayout;
		m_DescriptorImageInfo.imageView = m_ImageView;
		m_DescriptorImageInfo.sampler = m_Samper;
		ktxTexture_Destroy(ktxTexture);
	}

	VkImage VulkanTexture::CreateVkImage(uint32_t width, uint32_t height, int32_t mipLevels,
		VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling,
		VkImageUsageFlags usage, VkDeviceMemory& imageMemory, uint32_t arrayLayers)
	{
		auto device = VulkanContext::GetDevice().GetLogicalDevice();
		VkImage image = VK_NULL_HANDLE;

		VkImageCreateInfo imageCI = {};
		{
			imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageCI.imageType = VK_IMAGE_TYPE_2D;
			imageCI.format = format;
			imageCI.mipLevels = mipLevels;
			imageCI.extent.depth = 1;
			imageCI.arrayLayers = arrayLayers;
			imageCI.samples = numSamples;
			imageCI.tiling = tiling;
			imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageCI.extent = { (uint32_t)width, (uint32_t)height, 1 };
			imageCI.usage = usage;

			VK_CHECK_RESULT(vkCreateImage(device, &imageCI, nullptr, &image));
		}

		VkMemoryRequirements memReqs;
		vkGetImageMemoryRequirements(device,image, &memReqs);
		uint32_t typeIndex = VulkanContext::GetDevice().GetMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VulkanMemoryAllocator::Allocate(device, memReqs, &imageMemory, typeIndex);
		VK_CHECK_RESULT(vkBindImageMemory(device, image, imageMemory, 0));

		return image;
	}

	void VulkanTexture::CreateTexture(uint32_t width, uint32_t height, uint32_t mipMaps, const void* data, const TextureCreateInfo* info)
	{
		const VkDeviceSize size = width * height * 4;

		VulkanBuffer stagingBuffer;
		stagingBuffer.CreateBuffer(size, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
		stagingBuffer.SetData(data, size);

		m_Image = CreateVkImage(width, height, mipMaps, VK_SAMPLE_COUNT_1_BIT, m_Format,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, m_DeviceMemory);

		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.levelCount = 1;
		subresourceRange.layerCount = 1;

		CommandBufferStorage cmdStorage{};
		VulkanCommandBuffer::CreateCommandBuffer(&cmdStorage);
		{

			// Optimal image will be used as destination for the copy, so we must transfer from our initial undefined image layout to the transfer destination layout
			InsertImageMemoryBarrier(
				cmdStorage.Buffer,
				m_Image,
				0,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				subresourceRange);

			// Copy the first mip of the chain, remaining mips will be generated
			VkBufferImageCopy bufferCopyRegion = {};
			bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			bufferCopyRegion.imageSubresource.mipLevel = 0;
			bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
			bufferCopyRegion.imageSubresource.layerCount = 1;
			bufferCopyRegion.imageExtent.width = width;
			bufferCopyRegion.imageExtent.height = height;
			bufferCopyRegion.imageExtent.depth = 1;

			vkCmdCopyBufferToImage(cmdStorage.Buffer, stagingBuffer.GetBuffer(), m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferCopyRegion);

			// Transition first mip level to transfer source for read during blit
			InsertImageMemoryBarrier(
				cmdStorage.Buffer,
				m_Image,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_ACCESS_TRANSFER_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				subresourceRange);

			GenerateMipMaps(m_Image, cmdStorage.Buffer, width, height, mipMaps, subresourceRange);
		}
		VulkanCommandBuffer::ExecuteCommandBuffer(&cmdStorage);

		m_ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		CreateSamplerAndImageView(mipMaps, m_Format, info->bAnisotropyEnable);

		if (info->bImGUIHandle)
			m_Info->ImHandle = ImGui_ImplVulkan_AddTexture(m_DescriptorImageInfo);
	}

	void VulkanTexture::CreateFromBuffer(const void* data, VkDeviceSize size, uint32_t width, uint32_t height)
	{
		m_Info->Width = width;
		m_Info-> Height = height;

		VulkanBuffer stagingBuffer;
		stagingBuffer.CreateBuffer(size, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
		stagingBuffer.SetData(data, size);

		m_Image = CreateVkImage(width, height, 1, VK_SAMPLE_COUNT_1_BIT, m_Format,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, m_DeviceMemory);

		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.levelCount = 1;
		subresourceRange.layerCount = 1;

		CommandBufferStorage cmdStorage{};
		VulkanCommandBuffer::CreateCommandBuffer(&cmdStorage);
		{
			// Optimal image will be used as destination for the copy, so we must transfer from our initial undefined image layout to the transfer destination layout
			InsertImageMemoryBarrier(
				cmdStorage.Buffer,
				m_Image,
				0,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				subresourceRange);

			// Copy the first mip of the chain, remaining mips will be generated
			VkBufferImageCopy bufferCopyRegion = {};
			bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			bufferCopyRegion.imageSubresource.mipLevel = 0;
			bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
			bufferCopyRegion.imageSubresource.layerCount = 1;
			bufferCopyRegion.imageExtent.width = width;
			bufferCopyRegion.imageExtent.height = height;
			bufferCopyRegion.imageExtent.depth = 1;

			vkCmdCopyBufferToImage(cmdStorage.Buffer, stagingBuffer.GetBuffer(), m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferCopyRegion);

			// Transition first mip level to transfer source for read during blit
			InsertImageMemoryBarrier(
				cmdStorage.Buffer,
				m_Image,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_ACCESS_TRANSFER_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				subresourceRange);

		}
		VulkanCommandBuffer::ExecuteCommandBuffer(&cmdStorage);

		m_ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		CreateSamplerAndImageView(1, m_Format);
	}

	void VulkanTexture::GenerateMipMaps(VkImage image, VkCommandBuffer cmd, int32_t width, int32_t height, int32_t mipLevel, VkImageSubresourceRange& range)
	{
		// Copy down mips from n-1 to n
		for (int32_t i = 1; i < mipLevel; i++)
		{
			VkImageBlit imageBlit{};

			// Source
			imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBlit.srcSubresource.layerCount = 1;
			imageBlit.srcSubresource.mipLevel = i - 1;
			imageBlit.srcOffsets[1].x = int32_t(width >> (i - 1));
			imageBlit.srcOffsets[1].y = int32_t(height >> (i - 1));
			imageBlit.srcOffsets[1].z = 1;

			// Destination
			imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBlit.dstSubresource.layerCount = 1;
			imageBlit.dstSubresource.mipLevel = i;
			imageBlit.dstOffsets[1].x = int32_t(width >> i);
			imageBlit.dstOffsets[1].y = int32_t(height >> i);
			imageBlit.dstOffsets[1].z = 1;

			VkImageSubresourceRange mipSubRange = {};
			mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			mipSubRange.baseMipLevel = i;
			mipSubRange.levelCount = 1;
			mipSubRange.layerCount = 1;

			// Prepare current mip level as image blit destination
			InsertImageMemoryBarrier(
				cmd,
				m_Image,
				0,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				mipSubRange);

			// Blit from previous level
			vkCmdBlitImage(
				cmd,
				m_Image,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				m_Image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&imageBlit,
				VK_FILTER_LINEAR);

			// Prepare current mip level as image blit source for next level
			InsertImageMemoryBarrier(
				cmd,
				m_Image,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_ACCESS_TRANSFER_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				mipSubRange);
		}

		// After the loop, all mip layers are in TRANSFER_SRC layout, so transition all to SHADER_READ
		range.levelCount = mipLevel;
		InsertImageMemoryBarrier(
			cmd,
			m_Image,
			VK_ACCESS_TRANSFER_READ_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			range);
	}

	void VulkanTexture::InsertImageMemoryBarrier(VkCommandBuffer cmdbuffer, VkImage image, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkImageSubresourceRange subresourceRange)
	{
		VkImageMemoryBarrier imageMemoryBarrier = {};
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.srcAccessMask = srcAccessMask;
		imageMemoryBarrier.dstAccessMask = dstAccessMask;
		imageMemoryBarrier.oldLayout = oldImageLayout;
		imageMemoryBarrier.newLayout = newImageLayout;
		imageMemoryBarrier.image = image;
		imageMemoryBarrier.subresourceRange = subresourceRange;

		vkCmdPipelineBarrier(
			cmdbuffer,
			srcStageMask,
			dstStageMask,
			0,
			0, nullptr,
			0, nullptr,
			1, &imageMemoryBarrier);
	}

	void VulkanTexture::CreateSamplerAndImageView(uint32_t mipMaps, VkFormat format, bool anisotropy)
	{
		/// Samplers
	    /// https://vulkan-tutorial.com/Texture_mapping/Image_view_and_sampler

		VkSamplerCreateInfo samplerCI = {};
		{
			auto& device = VulkanContext::GetDevice();

			samplerCI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerCI.maxAnisotropy = 1.0f;
			samplerCI.magFilter = m_Filter;
			samplerCI.minFilter = m_Filter;
			samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerCI.addressModeU = m_AddressMode;
			samplerCI.addressModeV = m_AddressMode;
			samplerCI.addressModeW = m_AddressMode;
			samplerCI.compareOp = VK_COMPARE_OP_NEVER;
			samplerCI.mipLodBias = 0.0f;
			samplerCI.minLod = 0.0f;
			samplerCI.maxLod = static_cast<float>(mipMaps);
			samplerCI.maxAnisotropy = 1.0;
			samplerCI.borderColor = m_BorderColor;

			if (device.GetDeviceFeatures()->samplerAnisotropy && anisotropy)
			{
				samplerCI.maxAnisotropy = device.GetDeviceProperties()->limits.maxSamplerAnisotropy;
				samplerCI.anisotropyEnable = VK_TRUE;
			}

			VK_CHECK_RESULT(vkCreateSampler(m_Device, &samplerCI, nullptr, &m_Samper));
		}

		/// Image View
		/// https://vulkan-tutorial.com/Drawing_a_triangle/Presentation/Image_views

		VkImageViewCreateInfo imageViewCI = {};
		{
			imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
			imageViewCI.format = format;
			imageViewCI.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
			imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageViewCI.subresourceRange.baseMipLevel = 0;
			imageViewCI.subresourceRange.baseArrayLayer = 0;
			imageViewCI.subresourceRange.layerCount = 1;
			imageViewCI.subresourceRange.levelCount = mipMaps;
			imageViewCI.image = m_Image;

			VK_CHECK_RESULT(vkCreateImageView(m_Device, &imageViewCI, nullptr, &m_ImageView));
		}

		m_DescriptorImageInfo = {};
		m_DescriptorImageInfo.imageLayout = m_ImageLayout;
		m_DescriptorImageInfo.imageView = m_ImageView;
		m_DescriptorImageInfo.sampler = m_Samper;
	}

	void VulkanTexture::SetImageLayout(VkCommandBuffer cmdbuffer, VkImage image, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, VkImageSubresourceRange subresourceRange, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask)
	{
		VkImageMemoryBarrier imageMemoryBarrier = {};
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.oldLayout = oldImageLayout;
		imageMemoryBarrier.newLayout = newImageLayout;
		imageMemoryBarrier.image = image;
		imageMemoryBarrier.subresourceRange = subresourceRange;

		// Source layouts (old)
		// Source access mask controls actions that have to be finished on the old layout
		// before it will be transitioned to the new layout
		switch (oldImageLayout)
		{
		case VK_IMAGE_LAYOUT_UNDEFINED:
			// Image layout is undefined (or does not matter)
			// Only valid as initial layout
			// No flags required, listed only for completeness
			imageMemoryBarrier.srcAccessMask = 0;
			break;

		case VK_IMAGE_LAYOUT_PREINITIALIZED:
			// Image is preinitialized
			// Only valid as initial layout for linear images, preserves memory contents
			// Make sure host writes have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			// Image is a color attachment
			// Make sure any writes to the color buffer have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			// Image is a depth/stencil attachment
			// Make sure any writes to the depth/stencil buffer have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			// Image is a transfer source
			// Make sure any reads from the image have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			// Image is a transfer destination
			// Make sure any writes to the image have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			// Image is read by a shader
			// Make sure any shader reads from the image have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			break;
		default:
			// Other source layouts aren't handled (yet)
			break;
		}

		// Target layouts (new)
		// Destination access mask controls the dependency for the new image layout
		switch (newImageLayout)
		{
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			// Image will be used as a transfer destination
			// Make sure any writes to the image have been finished
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			// Image will be used as a transfer source
			// Make sure any reads from the image have been finished
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			break;

		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			// Image will be used as a color attachment
			// Make sure any writes to the color buffer have been finished
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			// Image layout will be used as a depth/stencil attachment
			// Make sure any writes to depth/stencil buffer have been finished
			imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			// Image will be read in a shader (sampler, input attachment)
			// Make sure any writes to the image have been finished
			if (imageMemoryBarrier.srcAccessMask == 0)
			{
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
			}
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			break;
		default:
			// Other source layouts aren't handled (yet)
			break;
		}

		// Put barrier inside setup command buffer
		vkCmdPipelineBarrier(
			cmdbuffer,
			srcStageMask,
			dstStageMask,
			0,
			0, nullptr,
			0, nullptr,
			1, &imageMemoryBarrier);
	}

	void VulkanTexture::SetImageLayout(VkCommandBuffer cmdbuffer, VkImage image, VkImageAspectFlags aspectMask, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask)
	{
		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = aspectMask;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = 1;
		subresourceRange.layerCount = 1;
		SetImageLayout(cmdbuffer, image, oldImageLayout, newImageLayout, subresourceRange, srcStageMask, dstStageMask);
	}

	VkFormat VulkanTexture::GetImageFormat(TextureFormat format)
	{
		switch (format)
		{
		case TextureFormat::R8_UNORM:                         return VK_FORMAT_R8_UNORM;
		case TextureFormat::R8G8B8A8_UNORM:                   return VK_FORMAT_R8G8B8A8_UNORM;
		case TextureFormat::B8G8R8A8_UNORM:                   return VK_FORMAT_B8G8R8A8_UNORM;
		case TextureFormat::R16G16B16A16_SFLOAT:              return VK_FORMAT_R16G16B16A16_SFLOAT;
		case TextureFormat::R32G32B32A32_SFLOAT:              return VK_FORMAT_R32G32B32A32_SFLOAT;
		default:                                              return VK_FORMAT_R8G8B8A8_UNORM;
		}
	}

	VkSamplerAddressMode VulkanTexture::GetVkSamplerAddressMode(AddressMode mode)
	{
		switch (mode)
		{
		case AddressMode::CLAMP_TO_BORDER: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		case AddressMode::CLAMP_TO_EDGE: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		case AddressMode::MIRRORED_REPEAT: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		case AddressMode::MIRROR_CLAMP_TO_EDGE: return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
		case AddressMode::REPEAT: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		}

		return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	}

	VkBorderColor VulkanTexture::GetVkBorderColor(BorderColor color)
	{
		switch (color)
		{
		case BorderColor::FLOAT_OPAQUE_BLACK: return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
		case BorderColor::FLOAT_OPAQUE_WHITE: return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		case BorderColor::FLOAT_TRANSPARENT_BLACK: return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		case BorderColor::INT_OPAQUE_BLACK: return VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		case BorderColor::INT_OPAQUE_WHITE: return VK_BORDER_COLOR_INT_OPAQUE_WHITE;
		case BorderColor::INT_TRANSPARENT_BLACK: return VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
		}

		return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	}

	const VkDescriptorImageInfo& VulkanTexture::GetVkDescriptorImageInfo() const
	{
		return m_DescriptorImageInfo;
	}

	VkImage VulkanTexture::GetVkImage() const
	{
		return m_Image;
	}
}
#endif