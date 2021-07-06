#include "stdafx.h"
#ifndef FROSTIUM_OPENGL_IMPL
#include "Vulkan/VulkanRenderPass.h"
#include "Vulkan/VulkanContext.h"
#include "Vulkan/VulkanFramebuffer.h"

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	void VulkanRenderPass::Create(FramebufferSpecification* framebufferSpec, RenderPassGenInfo* renderPassInfo, VkRenderPass& outPass)
	{
		VkDevice device = VulkanContext::GetDevice().GetLogicalDevice();

		std::vector<VkAttachmentReference> colorReferences(renderPassInfo->NumColorAttachments);
		std::vector<VkAttachmentReference> depthReferences(renderPassInfo->NumDepthAttachments);
		std::vector<VkAttachmentReference> resolveReferences(renderPassInfo->NumResolveAttachments);

		std::vector<VkAttachmentDescription> attachments(renderPassInfo->NumColorAttachments + renderPassInfo->NumDepthAttachments
			+ renderPassInfo->NumResolveAttachments);

		uint32_t lastAttachmentIndex = 0;
		for (uint32_t i = 0; i < renderPassInfo->NumColorAttachments; ++i)
		{
			VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			if (framebufferSpec->Attachments[i].bClearOp)
				loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;;

			VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			if (framebufferSpec->bTargetsSwapchain)
				layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			if (framebufferSpec->eMSAASampels != MSAASamples::SAMPLE_COUNT_1 && !framebufferSpec->bTargetsSwapchain)
				layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			// attachment
			attachments[lastAttachmentIndex].format = VulkanFramebuffer::GetAttachmentFormat(framebufferSpec->Attachments[i].Format);
			attachments[lastAttachmentIndex].samples = renderPassInfo->MSAASamples;
			attachments[lastAttachmentIndex].loadOp = loadOp;
			attachments[lastAttachmentIndex].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachments[lastAttachmentIndex].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachments[lastAttachmentIndex].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachments[lastAttachmentIndex].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachments[lastAttachmentIndex].finalLayout = layout;

			// referense
			colorReferences[i].attachment = lastAttachmentIndex;
			colorReferences[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			lastAttachmentIndex++;
		}


		for (uint32_t i = 0; i < renderPassInfo->NumResolveAttachments; ++i)
		{
			VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			if (framebufferSpec->bTargetsSwapchain)
				layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			if (!framebufferSpec->Attachments[i].bClearOp)
				loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;

			attachments[lastAttachmentIndex].format = VulkanFramebuffer::GetAttachmentFormat(framebufferSpec->Attachments[i].Format);
			attachments[lastAttachmentIndex].samples = VK_SAMPLE_COUNT_1_BIT;
			attachments[lastAttachmentIndex].loadOp = loadOp;
			attachments[lastAttachmentIndex].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachments[lastAttachmentIndex].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			attachments[lastAttachmentIndex].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachments[lastAttachmentIndex].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachments[lastAttachmentIndex].finalLayout = layout;

			resolveReferences[i].attachment = lastAttachmentIndex;
			resolveReferences[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			lastAttachmentIndex++;
		}

		for (uint32_t i = 0; i < renderPassInfo->NumDepthAttachments; ++i)
		{
			attachments[lastAttachmentIndex].format = renderPassInfo->DepthFormat;
			attachments[lastAttachmentIndex].samples = renderPassInfo->MSAASamples;
			attachments[lastAttachmentIndex].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachments[lastAttachmentIndex].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachments[lastAttachmentIndex].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachments[lastAttachmentIndex].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachments[lastAttachmentIndex].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachments[lastAttachmentIndex].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

			depthReferences[i].attachment = lastAttachmentIndex;
			depthReferences[i].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			lastAttachmentIndex++;
		}

		VkSubpassDescription subpass = {};
		{
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
			subpass.pColorAttachments = colorReferences.data();
			subpass.pDepthStencilAttachment = depthReferences.data();

			if (framebufferSpec->eMSAASampels != MSAASamples::SAMPLE_COUNT_1 && renderPassInfo->NumResolveAttachments > 0)
				subpass.pResolveAttachments = resolveReferences.data();
		}

		std::vector<VkSubpassDependency> dependencies;
		if (framebufferSpec->bAutoSync)
		{
			// Use subpass dependencies for attachment layout transitions
			dependencies.resize(2);

			dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[0].dstSubpass = 0;
			dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			dependencies[1].srcSubpass = 0;
			dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;


		}

		VkRenderPassCreateInfo renderPassCI = {};
		{
			renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassCI.attachmentCount = static_cast<uint32_t>(attachments.size());
			renderPassCI.pAttachments = attachments.data();
			renderPassCI.subpassCount = 1;
			renderPassCI.pSubpasses = &subpass;
			renderPassCI.dependencyCount = static_cast<uint32_t>(dependencies.size());
			renderPassCI.pDependencies = dependencies.data();
		}

		VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassCI, nullptr, &outPass));
	}
}
#endif