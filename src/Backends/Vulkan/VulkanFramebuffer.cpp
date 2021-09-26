#include "stdafx.h"
#ifndef FROSTIUM_OPENGL_IMPL
#include "Backends/Vulkan/VulkanFramebuffer.h"
#include "Backends/Vulkan/VulkanContext.h"
#include "Backends/Vulkan/VulkanRenderPass.h"
#include "Backends/Vulkan/VulkanMemoryAllocator.h"
#include "Backends/Vulkan/VulkanTexture.h"
#include "Backends/Vulkan/VulkanSemaphore.h"
#include "Primitives/Framebuffer.h"

#include <imgui/examples/imgui_impl_vulkan.h>

namespace SmolEngine
{
	VulkanFramebuffer::VulkanFramebuffer()
	{
	}

	VulkanFramebuffer::~VulkanFramebuffer()
	{
		FreeResources();

		if (m_RenderPass != VK_NULL_HANDLE)
			vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
	}

	bool VulkanFramebuffer::Create(uint32_t width, uint32_t height)
	{
		if (m_Spec->Attachments.size() > 1 && m_Spec->bTargetsSwapchain || m_Spec->Attachments.size() == 0)
			return false;

		m_Spec->Width = width;
		m_Spec->Height = height;

		uint32_t lastImageViewIndex = 0;
		uint32_t bufferSize = static_cast<uint32_t>(m_Spec->Attachments.size());
		uint32_t attachmentsCount = IsUseMSAA() ? (bufferSize * 2) + 1 : bufferSize + 1;

		m_Attachments.resize(bufferSize);
		m_ClearValues.resize(attachmentsCount);
		m_ClearAttachments.resize(attachmentsCount);
		std::vector<VkImageView> attachments(attachmentsCount);

		CommandBufferStorage cmdStorage{};
		VulkanCommandBuffer::CreateCommandBuffer(&cmdStorage);

		// Sampler
		CreateSampler(m_Spec->eFiltering == ImageFilter::LINEAR ? VK_FILTER_LINEAR: VK_FILTER_NEAREST);

		// Color Attachments
		for (uint32_t i = 0; i < bufferSize; ++i)
		{
			auto& info = m_Spec->Attachments[i];
			auto& vkInfo = m_Attachments[i];

			VkImageUsageFlags usage = IsUseMSAA() ? VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT :
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

			AddAttachment(width, height, m_MSAASamples, usage, GetAttachmentFormat(info.Format),
				vkInfo.AttachmentVkInfo.image, vkInfo.AttachmentVkInfo.view, vkInfo.AttachmentVkInfo.mem);

			if (!IsUseMSAA())
			{
				vkInfo.ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				vkInfo.ImageInfo.imageView = vkInfo.AttachmentVkInfo.view;
				vkInfo.ImageInfo.sampler = m_Sampler;

				if (m_Spec->bUsedByImGui)
				{
					vkInfo.ImGuiID = ImGui_ImplVulkan_AddTexture(vkInfo.ImageInfo);
				}

				VulkanTexture::SetImageLayout(cmdStorage.Buffer, vkInfo.AttachmentVkInfo.image,
					VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			}

			attachments[lastImageViewIndex] = vkInfo.AttachmentVkInfo.view;

			m_ClearValues[lastImageViewIndex].color = { { info.ClearColor.r,info.ClearColor.g, info.ClearColor.b, info.ClearColor.a } };
			m_ClearAttachments[lastImageViewIndex].aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			m_ClearAttachments[lastImageViewIndex].clearValue = m_ClearValues[lastImageViewIndex];
			m_ClearAttachments[lastImageViewIndex].colorAttachment = lastImageViewIndex;

			if (!info.Name.empty())
				m_AttachmentsMap[info.Name] = i;

			lastImageViewIndex++;
		}

		// Use swapchain image as resolve attachment - no need to create attachment 
		if (IsUseMSAA() && m_Spec->bTargetsSwapchain)
			lastImageViewIndex++;

		// Create resolve attachment if MSAA enabled and swapchain is not target
		if (IsUseMSAA() && !m_Spec->bTargetsSwapchain)
		{
			m_ResolveAttachments.resize(bufferSize);
			for (uint32_t i = 0; i < bufferSize; ++i)
			{
				auto& resolve = m_ResolveAttachments[i];
				auto& info = m_Spec->Attachments[i];
				VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

				AddAttachment(width, height, VK_SAMPLE_COUNT_1_BIT, usage, GetAttachmentFormat(info.Format),
					resolve.AttachmentVkInfo.image, resolve.AttachmentVkInfo.view,
					resolve.AttachmentVkInfo.mem);

				resolve.ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				resolve.ImageInfo.imageView = resolve.AttachmentVkInfo.view;
				resolve.ImageInfo.sampler = m_Sampler;

				VulkanTexture::SetImageLayout(cmdStorage.Buffer, resolve.AttachmentVkInfo.image,
					VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

				if (m_Spec->bUsedByImGui)
					resolve.ImGuiID = ImGui_ImplVulkan_AddTexture(resolve.ImageInfo);

				attachments[lastImageViewIndex] = resolve.AttachmentVkInfo.view;

				m_ClearValues[lastImageViewIndex].color = { { info.ClearColor.r,info.ClearColor.g, info.ClearColor.b, info.ClearColor.a } };
				m_ClearAttachments[lastImageViewIndex].aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				m_ClearAttachments[lastImageViewIndex].clearValue = m_ClearValues[lastImageViewIndex];

				if (!info.Name.empty())
					m_AttachmentsMap[info.Name + "_resolve"] = i;

				lastImageViewIndex++;
			}
		}

		if (m_Spec->bTargetsSwapchain)
		{
			m_ClearValues[lastImageViewIndex].color = { { 0.1f, 0.1f, 0.1f, 1.0f} };
			m_ClearAttachments[lastImageViewIndex].aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			m_ClearAttachments[lastImageViewIndex].clearValue = m_ClearValues[lastImageViewIndex];
		}

		// Depth stencil attachment
		{
			VkImageUsageFlags usage = IsUseMSAA() ? VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
				: VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

			VkImageAspectFlags imageAspect = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
			AddAttachment(width, height, m_MSAASamples, usage, m_DepthFormat,
				m_DepthAttachment.AttachmentVkInfo.image, m_DepthAttachment.AttachmentVkInfo.view, m_DepthAttachment.AttachmentVkInfo.mem, imageAspect);

			attachments[lastImageViewIndex] = m_DepthAttachment.AttachmentVkInfo.view;

			m_ClearValues[lastImageViewIndex].depthStencil = { 1.0f, 0 };
			m_ClearAttachments[lastImageViewIndex].aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
			m_ClearAttachments[lastImageViewIndex].clearValue.depthStencil = { 1.0f, 0 };
		}

		// Render pass
		if (m_RenderPass == VK_NULL_HANDLE)
		{
			RenderPassGenInfo renderPassGenInfo = {};
			{
				renderPassGenInfo.ColorFormat = m_ColorFormat;
				renderPassGenInfo.DepthFormat = m_DepthFormat;
				renderPassGenInfo.MSAASamples = m_MSAASamples;
				renderPassGenInfo.NumColorAttachments = static_cast<uint32_t>(m_Attachments.size());
				renderPassGenInfo.NumResolveAttachments = m_Spec->bTargetsSwapchain && IsUseMSAA() ? 1 : static_cast<uint32_t>(m_ResolveAttachments.size());
				renderPassGenInfo.NumDepthAttachments = 1;
			}

			VulkanRenderPass::Create(m_Spec, &renderPassGenInfo, m_RenderPass);
		}

		VkFramebufferCreateInfo fbufCreateInfo = {};
		{
			fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			fbufCreateInfo.pNext = NULL;
			fbufCreateInfo.renderPass = m_RenderPass;
			fbufCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			fbufCreateInfo.pAttachments = attachments.data();
			fbufCreateInfo.width = m_Spec->Width;
			fbufCreateInfo.height = m_Spec->Height;
			fbufCreateInfo.layers = 1;
		}

		if (m_Spec->bTargetsSwapchain)
		{
			uint32_t count = VulkanContext::GetSwapchain().m_ImageCount;
			m_VkFrameBuffers.resize(count);
			for (uint32_t i = 0; i < count; ++i)
			{
				uint32_t index = IsUseMSAA() ? 1 : 0;
				attachments[index] = VulkanContext::GetSwapchain().m_Buffers[i].View;
				VK_CHECK_RESULT(vkCreateFramebuffer(m_Device, &fbufCreateInfo, nullptr, &m_VkFrameBuffers[i]));
			}

			return true;
		}

		VulkanCommandBuffer::ExecuteCommandBuffer(&cmdStorage);

		m_VkFrameBuffers.resize(1);
		VK_CHECK_RESULT(vkCreateFramebuffer(m_Device, &fbufCreateInfo, nullptr, &m_VkFrameBuffers[0]));
		return true;
	}

	bool VulkanFramebuffer::CreateShadow(uint32_t width, uint32_t height)
	{
		// Depth 
		{
			VkImageUsageFlags usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			VkImageAspectFlags imageAspect = VK_IMAGE_ASPECT_DEPTH_BIT;

			AddAttachment(width, height, VK_SAMPLE_COUNT_1_BIT, usage, m_DepthFormat,
				m_DepthAttachment.AttachmentVkInfo.image, m_DepthAttachment.AttachmentVkInfo.view, m_DepthAttachment.AttachmentVkInfo.mem, imageAspect);
		}

		CommandBufferStorage cmdStorage{};
		VulkanCommandBuffer::CreateCommandBuffer(&cmdStorage);
		VulkanTexture::SetImageLayout(cmdStorage.Buffer, m_DepthAttachment.AttachmentVkInfo.image,
			VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
		VulkanCommandBuffer::ExecuteCommandBuffer(&cmdStorage);

		// Sampler
		{
			VkFilter shadowmap_filter = IsFormatIsFilterable(VulkanContext::GetDevice().GetPhysicalDevice(), m_DepthFormat, VK_IMAGE_TILING_OPTIMAL) ?
				VK_FILTER_LINEAR : VK_FILTER_NEAREST;

			VkSamplerCreateInfo sampler = {};
			sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			sampler.maxAnisotropy = 1.0f;
			sampler.magFilter = shadowmap_filter;
			sampler.minFilter = shadowmap_filter;
			sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			sampler.addressModeV = sampler.addressModeU;
			sampler.addressModeW = sampler.addressModeU;
			sampler.mipLodBias = 0.0f;
			sampler.maxAnisotropy = 1.0f;
			sampler.minLod = 0.0f;
			sampler.maxLod = 1.0f;
			sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
			VK_CHECK_RESULT(vkCreateSampler(m_Device, &sampler, nullptr, &m_Sampler));
		}

		// Render Pass
		{
			VkAttachmentDescription attachmentDescription{};
			attachmentDescription.format = m_DepthFormat;
			attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
			attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;							// Clear depth at beginning of the render pass
			attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;						// We will read from depth, so it's important to store the depth attachment results
			attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;					// We don't care about initial layout of the attachment
			attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;// Attachment will be transitioned to shader read at render pass end

			VkAttachmentReference depthReference = {};
			depthReference.attachment = 0;
			depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;			// Attachment will be used as depth/stencil during render pass

			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 0;													// No color attachments
			subpass.pDepthStencilAttachment = &depthReference;									// Reference to our depth attachment

			VkRenderPassCreateInfo renderPassCreateInfo = {};
			renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassCreateInfo.attachmentCount = 1;
			renderPassCreateInfo.pAttachments = &attachmentDescription;
			renderPassCreateInfo.subpassCount = 1;
			renderPassCreateInfo.pSubpasses = &subpass;

			VK_CHECK_RESULT(vkCreateRenderPass(m_Device, &renderPassCreateInfo, nullptr, &m_RenderPass));
		}

		// Framebuffer
		{
			VkFramebufferCreateInfo fbufCreateInfo = {};
			fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			fbufCreateInfo.renderPass = m_RenderPass;
			fbufCreateInfo.attachmentCount = 1;
			fbufCreateInfo.pAttachments = &m_DepthAttachment.AttachmentVkInfo.view;
			fbufCreateInfo.width = width;
			fbufCreateInfo.height = height;
			fbufCreateInfo.layers = 1;

			m_VkFrameBuffers.resize(1);
			VK_CHECK_RESULT(vkCreateFramebuffer(m_Device, &fbufCreateInfo, nullptr, &m_VkFrameBuffers[0]));
		}


		m_DepthAttachment.ImageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		m_DepthAttachment.ImageInfo.sampler = m_Sampler;
		m_DepthAttachment.ImageInfo.imageView = m_DepthAttachment.AttachmentVkInfo.view;

		m_ClearValues.resize(1);
		m_ClearAttachments.resize(1);

		m_ClearValues[0].depthStencil = { 1.0f, 0 };
		m_ClearAttachments[0].aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		m_ClearAttachments[0].clearValue.depthStencil = { 1.0f, 0 };

		return true;
	}

	bool VulkanFramebuffer::CreateCopyFramebuffer(uint32_t width, uint32_t height)
	{
		
		m_Attachments.resize(1);
		VkImageCreateInfo imageCreateInfo = {};
		CommandBufferStorage cmdStorage{};
		VulkanCommandBuffer::CreateCommandBuffer(&cmdStorage);
		VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

		// Color
		{
			auto& attachment = m_Attachments[0];

			// Color attachment
			imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
			imageCreateInfo.format = format;
			imageCreateInfo.extent.width = width;
			imageCreateInfo.extent.height = height;
			imageCreateInfo.extent.depth = 1;
			imageCreateInfo.mipLevels = 1;
			imageCreateInfo.arrayLayers = 1;
			imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			// Image of the framebuffer is blit source
			imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
			imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			VK_CHECK_RESULT(vkCreateImage(m_Device, &imageCreateInfo, nullptr, &attachment.AttachmentVkInfo.image));

			VkMemoryRequirements memReqs;
			vkGetImageMemoryRequirements(m_Device, attachment.AttachmentVkInfo.image, &memReqs);
			uint32_t typeIndex = VulkanContext::GetDevice().GetMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			VulkanMemoryAllocator::Allocate(m_Device, memReqs, &attachment.AttachmentVkInfo.mem, typeIndex);
			VK_CHECK_RESULT(vkBindImageMemory(m_Device, attachment.AttachmentVkInfo.image, attachment.AttachmentVkInfo.mem, 0));

			VkImageViewCreateInfo colorImageView = {};
			colorImageView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			colorImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
			colorImageView.format = format;
			colorImageView.flags = 0;
			colorImageView.subresourceRange = {};
			colorImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			colorImageView.subresourceRange.baseMipLevel = 0;
			colorImageView.subresourceRange.levelCount = 1;
			colorImageView.subresourceRange.baseArrayLayer = 0;
			colorImageView.subresourceRange.layerCount = 1;
			colorImageView.image = attachment.AttachmentVkInfo.image;

			VulkanTexture::SetImageLayout(cmdStorage.Buffer, attachment.AttachmentVkInfo.image,
				VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

			VK_CHECK_RESULT(vkCreateImageView(m_Device, &colorImageView, nullptr, &attachment.AttachmentVkInfo.view));

		}

		// Depth
		{
			// Depth stencil attachment
			imageCreateInfo.format = m_DepthFormat;
			imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

			VK_CHECK_RESULT(vkCreateImage(m_Device, &imageCreateInfo, nullptr, &m_DepthAttachment.AttachmentVkInfo.image));
			VkMemoryRequirements memReqs;
			vkGetImageMemoryRequirements(m_Device, m_DepthAttachment.AttachmentVkInfo.image, &memReqs);
			uint32_t typeIndex = VulkanContext::GetDevice().GetMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			VulkanMemoryAllocator::Allocate(m_Device, memReqs, &m_DepthAttachment.AttachmentVkInfo.mem, typeIndex);
			VK_CHECK_RESULT(vkBindImageMemory(m_Device, m_DepthAttachment.AttachmentVkInfo.image, m_DepthAttachment.AttachmentVkInfo.mem, 0));

			VkImageViewCreateInfo depthStencilView = {};
			depthStencilView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
			depthStencilView.format = m_DepthFormat;
			depthStencilView.flags = 0;
			depthStencilView.subresourceRange = {};
			depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
			depthStencilView.subresourceRange.baseMipLevel = 0;
			depthStencilView.subresourceRange.levelCount = 1;
			depthStencilView.subresourceRange.baseArrayLayer = 0;
			depthStencilView.subresourceRange.layerCount = 1;
			depthStencilView.image = m_DepthAttachment.AttachmentVkInfo.image;

			VulkanTexture::SetImageLayout(cmdStorage.Buffer, m_DepthAttachment.AttachmentVkInfo.image,
				VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

			VK_CHECK_RESULT(vkCreateImageView(m_Device, &depthStencilView, nullptr, &m_DepthAttachment.AttachmentVkInfo.view));
		}

		VulkanCommandBuffer::ExecuteCommandBuffer(&cmdStorage);

		// Render pass
		{
			VkAttachmentDescription osAttachments[2] = {};

			osAttachments[0].format = format;
			osAttachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
			osAttachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			osAttachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			osAttachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			osAttachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			osAttachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			osAttachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			// Depth attachment
			osAttachments[1].format = m_DepthFormat;
			osAttachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
			osAttachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			osAttachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			osAttachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			osAttachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			osAttachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			osAttachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkAttachmentReference colorReference = {};
			colorReference.attachment = 0;
			colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkAttachmentReference depthReference = {};
			depthReference.attachment = 1;
			depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &colorReference;
			subpass.pDepthStencilAttachment = &depthReference;

			VkRenderPassCreateInfo renderPassCreateInfo = {};
			renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassCreateInfo.attachmentCount = 2;
			renderPassCreateInfo.pAttachments = osAttachments;
			renderPassCreateInfo.subpassCount = 1;
			renderPassCreateInfo.pSubpasses = &subpass;

			VK_CHECK_RESULT(vkCreateRenderPass(m_Device, &renderPassCreateInfo, nullptr, &m_RenderPass));
		}

		// Frame buffer
		{
			VkImageView attachments[2];
			attachments[0] = m_Attachments[0].AttachmentVkInfo.view;
			attachments[1] = m_DepthAttachment.AttachmentVkInfo.view;

			m_VkFrameBuffers.resize(1);
			VkFramebufferCreateInfo fbufCreateInfo = {};
			fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			fbufCreateInfo.renderPass = m_RenderPass;
			fbufCreateInfo.attachmentCount = 2;
			fbufCreateInfo.pAttachments = attachments;
			fbufCreateInfo.width = width;
			fbufCreateInfo.height = height;
			fbufCreateInfo.layers = 1;

			VK_CHECK_RESULT(vkCreateFramebuffer(m_Device, &fbufCreateInfo, nullptr, &m_VkFrameBuffers[0]));
		}

		m_ClearValues.resize(2);
		m_ClearAttachments.resize(2);

		m_ClearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f} };
		m_ClearAttachments[0].aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		m_ClearAttachments[0].clearValue.color = { { 0.0f, 0.0f, 0.0f, 1.0f} };
		m_ClearAttachments[0].colorAttachment = 0;


		m_ClearValues[1].depthStencil = { 1.0f, 0 };
		m_ClearAttachments[1].aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		m_ClearAttachments[1].clearValue.depthStencil = { 1.0f, 0 };

		return true;
	}

	void VulkanFramebuffer::CreateSampler(VkFilter filter)
	{
		VkSamplerCreateInfo samplerCI = {};
		{
			auto& device = VulkanContext::GetDevice();

			samplerCI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerCI.magFilter = filter;
			samplerCI.minFilter = filter;
			samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerCI.addressModeV = samplerCI.addressModeU;
			samplerCI.addressModeW = samplerCI.addressModeU;
			samplerCI.mipLodBias = 0.0f;
			samplerCI.maxAnisotropy = 1.0f;
			samplerCI.minLod = 0.0f;
			samplerCI.maxLod = 1.0f;
			samplerCI.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
			samplerCI.maxAnisotropy = 1.0f;
			if (device.GetDeviceFeatures()->samplerAnisotropy)
			{
				samplerCI.maxAnisotropy = device.GetDeviceProperties()->limits.maxSamplerAnisotropy;
				samplerCI.anisotropyEnable = VK_TRUE;
			}

			VK_CHECK_RESULT(vkCreateSampler(m_Device, &samplerCI, nullptr, &m_Sampler));
		}
	}

	bool VulkanFramebuffer::Init(FramebufferSpecification* data)
	{
		m_Spec = data;
		m_MSAASamples = GetVkMSAASamples(data->eMSAASampels);

		m_Device = VulkanContext::GetDevice().GetLogicalDevice();
		m_ColorFormat = VulkanContext::GetSwapchain().GetColorFormat();
		m_DepthFormat = VulkanContext::GetSwapchain().GetDepthFormat();

		switch (data->eSpecialisation)
		{
		case FramebufferSpecialisation::None:
		{
			return Create(data->Width, data->Height);
		}
		case FramebufferSpecialisation::CopyBuffer:
		{
			return CreateCopyFramebuffer(data->Width, data->Height);
		}
		case FramebufferSpecialisation::ShadowMap:
		{
			return CreateShadow(data->Width, data->Height);
		}
		default:
			return false;
		}
	}

	void VulkanFramebuffer::SetSize(uint32_t width, uint32_t height)
	{
		if (m_Spec->bResizable)
		{
			FreeResources();
			Create(width, height);
		}
	}

	void VulkanFramebuffer::FreeResources()
	{
		FreeAttachment(m_DepthAttachment);
		for (auto& color : m_Attachments)
			FreeAttachment(color);

		for(auto& resolve: m_ResolveAttachments)
			FreeAttachment(resolve);

		if (m_Sampler != VK_NULL_HANDLE)
		{
			vkDestroySampler(m_Device, m_Sampler, nullptr);
		}

		for (auto& fb : m_VkFrameBuffers)
		{
			if (fb != VK_NULL_HANDLE)
			{
				vkDestroyFramebuffer(m_Device, fb, nullptr);
			}
		}
	}

	void VulkanFramebuffer::AddAttachment(uint32_t width, uint32_t height,
		VkSampleCountFlagBits samples, VkImageUsageFlags imageUsage, VkFormat format,
		VkImage& image, VkImageView& imageView, VkDeviceMemory& mem, VkImageAspectFlags imageAspect)
	{
		image = VulkanTexture::CreateVkImage(width, height,
			1,
			samples,
			format,
			VK_IMAGE_TILING_OPTIMAL,
			imageUsage, mem);

		VkImageViewCreateInfo colorImageViewCI = {};
		{
			colorImageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			colorImageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
			colorImageViewCI.format = format;
			colorImageViewCI.components.r = VK_COMPONENT_SWIZZLE_R;
			colorImageViewCI.components.g = VK_COMPONENT_SWIZZLE_G;
			colorImageViewCI.components.b = VK_COMPONENT_SWIZZLE_B;
			colorImageViewCI.components.a = VK_COMPONENT_SWIZZLE_A;
			colorImageViewCI.subresourceRange = {};
			colorImageViewCI.subresourceRange.aspectMask = imageAspect;
			colorImageViewCI.subresourceRange.baseMipLevel = 0;
			colorImageViewCI.subresourceRange.baseArrayLayer = 0;
			colorImageViewCI.subresourceRange.levelCount = 1;
			colorImageViewCI.subresourceRange.layerCount = 1;
			colorImageViewCI.image = image;

			VK_CHECK_RESULT(vkCreateImageView(m_Device, &colorImageViewCI, nullptr, &imageView));
		}
	}

	void VulkanFramebuffer::FreeAttachment(Attachment& framebuffer)
	{
		if (framebuffer.AttachmentVkInfo.view != VK_NULL_HANDLE)
			vkDestroyImageView(m_Device, framebuffer.AttachmentVkInfo.view, nullptr);

		if (framebuffer.AttachmentVkInfo.image != VK_NULL_HANDLE)
			vkDestroyImage(m_Device, framebuffer.AttachmentVkInfo.image, nullptr);

		if (framebuffer.AttachmentVkInfo.mem != VK_NULL_HANDLE)
			vkFreeMemory(m_Device, framebuffer.AttachmentVkInfo.mem, nullptr);
	}

	bool VulkanFramebuffer::IsUseMSAA()
	{
		return m_Spec->eMSAASampels != MSAASamples::SAMPLE_COUNT_1;
	}

	VkBool32 VulkanFramebuffer::IsFormatIsFilterable(VkPhysicalDevice physicalDevice, VkFormat format, VkImageTiling tiling)
	{
		VkFormatProperties formatProps;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);

		if (tiling == VK_IMAGE_TILING_OPTIMAL)
			return formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;

		if (tiling == VK_IMAGE_TILING_LINEAR)
			return formatProps.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;

		return false;
	}

	VkSampleCountFlagBits VulkanFramebuffer::GetVkMSAASamples(MSAASamples samples)
	{
		switch (samples)
		{
		case MSAASamples::SAMPLE_COUNT_1:                return VK_SAMPLE_COUNT_1_BIT;
		case MSAASamples::SAMPLE_COUNT_2:                return VK_SAMPLE_COUNT_2_BIT;
		case MSAASamples::SAMPLE_COUNT_4:                return VK_SAMPLE_COUNT_4_BIT;
		case MSAASamples::SAMPLE_COUNT_8:                return VK_SAMPLE_COUNT_8_BIT;
		case MSAASamples::SAMPLE_COUNT_16:               return VK_SAMPLE_COUNT_16_BIT;
		case MSAASamples::SAMPLE_COUNT_MAX_SUPPORTED:    return VulkanContext::GetDevice().GetMSAASamplesCount();
		default:                                         return VK_SAMPLE_COUNT_1_BIT;
		}
	}

	VkFormat VulkanFramebuffer::GetAttachmentFormat(AttachmentFormat format)
	{
		switch (format)
		{

		case AttachmentFormat::UNORM_8:			         return VK_FORMAT_R8_UNORM;
		case AttachmentFormat::UNORM2_8: 			     return VK_FORMAT_R8G8_UNORM;
		case AttachmentFormat::UNORM3_8: 			     return VK_FORMAT_R8G8B8_UNORM;
		case AttachmentFormat::UNORM4_8: 			     return VK_FORMAT_R8G8B8A8_UNORM;
													     
		case AttachmentFormat::UNORM_16:			     return VK_FORMAT_R16_UNORM;
		case AttachmentFormat::UNORM2_16: 			     return VK_FORMAT_R16G16_UNORM;
		case AttachmentFormat::UNORM3_16: 			     return VK_FORMAT_R16G16B16_UNORM;
		case AttachmentFormat::UNORM4_16: 			     return VK_FORMAT_R16G16B16A16_UNORM;
													     
		case AttachmentFormat::SFloat_16: 			     return VK_FORMAT_R16_SFLOAT;
		case AttachmentFormat::SFloat2_16: 			     return VK_FORMAT_R16G16_SFLOAT;
		case AttachmentFormat::SFloat3_16: 			     return VK_FORMAT_R16G16B16_SFLOAT;
		case AttachmentFormat::SFloat4_16: 			     return VK_FORMAT_R16G16B16A16_SFLOAT;
													     
		case AttachmentFormat::SFloat_32: 			     return VK_FORMAT_R32_SFLOAT;
		case AttachmentFormat::SFloat2_32: 			     return VK_FORMAT_R32G32_SFLOAT;
		case AttachmentFormat::SFloat3_32: 			     return VK_FORMAT_R32G32B32_SFLOAT;
		case AttachmentFormat::SFloat4_32: 			     return VK_FORMAT_R32G32B32A32_SFLOAT;
													     
		case AttachmentFormat::SInt_8: 			         return VK_FORMAT_R8_SINT;
		case AttachmentFormat::SInt2_8:			         return VK_FORMAT_R8G8_SINT;
		case AttachmentFormat::SInt3_8:			         return VK_FORMAT_R8G8B8_SINT;
		case AttachmentFormat::SInt4_8:			         return VK_FORMAT_R8G8B8A8_SINT;
												         
		case AttachmentFormat::SInt_16: 			     return VK_FORMAT_R16_SINT;
		case AttachmentFormat::SInt2_16:			     return VK_FORMAT_R16G16_SINT;
		case AttachmentFormat::SInt3_16:			     return VK_FORMAT_R16G16B16_SINT;
		case AttachmentFormat::SInt4_16:			     return VK_FORMAT_R16G16B16A16_SINT;
												         
		case AttachmentFormat::SInt_32: 			     return VK_FORMAT_R32_SINT;
		case AttachmentFormat::SInt2_32:			     return VK_FORMAT_R32G32_SINT;
		case AttachmentFormat::SInt3_32:			     return VK_FORMAT_R32G32B32_SINT;
		case AttachmentFormat::SInt4_32:			     return VK_FORMAT_R32G32B32A32_SINT;
													     
		case AttachmentFormat::Color:			         return VulkanContext::GetSwapchain().GetColorFormat();
		case AttachmentFormat::Depth:                    return VulkanContext::GetSwapchain().GetDepthFormat();
		default:
			break;
		}

		return VulkanContext::GetSwapchain().GetColorFormat();
	}

	VkRenderPass VulkanFramebuffer::GetRenderPass() const
	{
		return m_RenderPass;
	}

	VkSampleCountFlagBits VulkanFramebuffer::GetMSAASamples() const
	{
		return m_MSAASamples;
	}

	const uint32_t VulkanFramebuffer::GetAttachmentCount() const
	{
		return static_cast<uint32_t>(m_Attachments.size());
	}

	Attachment* VulkanFramebuffer::GetAttachment(uint32_t index)
	{
		if (IsUseMSAA())
			return &m_ResolveAttachments[index];

		return &m_Attachments[index];
	}

	Attachment* VulkanFramebuffer::GetAttachment(const std::string& name)
	{
		if (name == "Depth_Attachment")
			return &m_DepthAttachment;

		if (IsUseMSAA())
		{
			const auto& it = m_AttachmentsMap.find(name + "_resolve");
			if (it != m_AttachmentsMap.end())
				return &m_ResolveAttachments[it->second];

			return nullptr;
		}

		const auto& it = m_AttachmentsMap.find(name);
		if (it != m_AttachmentsMap.end())
			return &m_Attachments[it->second];

		return nullptr;
	}

	Attachment* VulkanFramebuffer::GetDethAttachment()
	{
		return &m_DepthAttachment;
	}

	void VulkanFramebuffer::SetClearColors(const glm::vec4& clearColors)
	{
		for (auto& clearAttachment : m_ClearAttachments)
		{
			clearAttachment.clearValue = { { clearColors.r,  clearColors.g,  clearColors.b,  clearColors.a } };
		}
	}

	const std::vector<VkClearAttachment>& VulkanFramebuffer::GetClearAttachments() const
	{
		return m_ClearAttachments;
	}

	const std::vector<VkClearValue>& VulkanFramebuffer::GetClearValues() const
	{
		return m_ClearValues;
	}

	const VkFramebuffer VulkanFramebuffer::GetCurrentVkFramebuffer() const
	{
		uint32_t index;
		m_Spec->bTargetsSwapchain ? index = VulkanContext::GetSwapchain().GetCurrentBufferIndex() :
			index = 0;
		return m_VkFrameBuffers[index];
	}

	const VkFramebuffer VulkanFramebuffer::GetVkFramebuffer(uint32_t index) const
	{
		return m_VkFrameBuffers[index];
	}
}
#endif