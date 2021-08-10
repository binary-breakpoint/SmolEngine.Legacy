#include "stdafx.h"
#ifndef FROSTIUM_OPENGL_IMPL
#include "Backends/Vulkan/GUI/ImGuiVulkanImpl.h"
#include "Backends/Vulkan/VulkanContext.h"
#include "Backends/Vulkan/VulkanStagingBuffer.h"
#include "Backends/Vulkan/VulkanSwapchain.h"
#include "Backends/Vulkan/VulkanCommandBuffer.h"

#include <imgui/imgui.h>
#include <GLFW/glfw3.h>

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	void ImGuiVulkanImpl::Init()
	{
		g_Device = VulkanContext::GetDevice().GetLogicalDevice();

		// Create Descriptor Pool
		{
			VkDescriptorPoolSize pool_sizes[] =
			{
				{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
				{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
				{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
			};

			VkDescriptorPoolCreateInfo pool_info = {};
			pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
			pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
			pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
			pool_info.pPoolSizes = pool_sizes;
			VK_CHECK_RESULT(vkCreateDescriptorPool(g_Device, &pool_info, nullptr, &g_DescriptorPool));
		}

		VkPipelineCacheCreateInfo pipelineCacheCI = {};
		{
			pipelineCacheCI.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		}

		VK_CHECK_RESULT(vkCreatePipelineCache(g_Device, &pipelineCacheCI, nullptr, &g_PipelineCache));
	}

	void ImGuiVulkanImpl::Reset()
	{

	}

	void ImGuiVulkanImpl::Draw(Framebuffer* target)
	{
		if (ImGui::GetDrawData()->CmdListsCount > 0)
		{
			CommandBufferStorage cmdStorage;
			VulkanCommandBuffer::CreateCommandBuffer(&cmdStorage);
			{
				uint32_t width = target->GetSpecification().Width;
				uint32_t height = target->GetSpecification().Height;

				// Set clear values for all framebuffer attachments with loadOp set to clear
				// We use two attachments (color and depth) that are cleared at the start of the subpass and as such we need to set clear values for both
				VkClearValue clearValues[2];
				clearValues[0].color = { 0.1f, 0.1f, 0.1f, 1.0f };
				clearValues[1].depthStencil = { 1.0f, 0 };

				VkRenderPassBeginInfo renderPassBeginInfo = {};
				renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				renderPassBeginInfo.pNext = nullptr;
				renderPassBeginInfo.renderPass = target->GetVulkanFramebuffer().GetRenderPass();
				renderPassBeginInfo.renderArea.offset.x = 0;
				renderPassBeginInfo.renderArea.offset.y = 0;
				renderPassBeginInfo.renderArea.extent.width = width;
				renderPassBeginInfo.renderArea.extent.height = height;
				renderPassBeginInfo.clearValueCount = 2;
				renderPassBeginInfo.pClearValues = clearValues;
				// Set target frame buffer
				renderPassBeginInfo.framebuffer = target->GetVulkanFramebuffer().GetCurrentVkFramebuffer();

				// Start the first sub pass specified in our default render pass setup by the base class
				// This will clear the color and depth attachment
				vkCmdBeginRenderPass(cmdStorage.Buffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

				// Update dynamic viewport state
				VkViewport viewport = {};
				viewport.x = 0;
				viewport.y = (float)height;
				viewport.height = -(float)height;
				viewport.width = (float)width;
				viewport.minDepth = (float)0.0f;
				viewport.maxDepth = (float)1.0f;
				vkCmdSetViewport(cmdStorage.Buffer, 0, 1, &viewport);

				// Update dynamic scissor state
				VkRect2D scissor = {};
				scissor.extent.width = width;
				scissor.extent.height = height;
				scissor.offset.x = 0;
				scissor.offset.y = 0;
				vkCmdSetScissor(cmdStorage.Buffer, 0, 1, &scissor);

				// Draw Imgui
				ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdStorage.Buffer);
				vkCmdEndRenderPass(cmdStorage.Buffer);
			}
			VulkanCommandBuffer::ExecuteCommandBuffer(&cmdStorage);
		}
	}

	void ImGuiVulkanImpl::Draw(VulkanSwapchain* target)
	{
		if (ImGui::GetDrawData()->CmdListsCount > 0)
		{
			auto cmd = VulkanContext::GetCurrentVkCmdBuffer();
			uint32_t width = target->GetWidth();
			uint32_t height = target->GetHeight();

			// Set clear values for all framebuffer attachments with loadOp set to clear
			// We use two attachments (color and depth) that are cleared at the start of the subpass and as such we need to set clear values for both
			VkClearValue clearValues[2];
			clearValues[0].color = { 0.1f, 0.1f, 0.1f, 1.0f };
			clearValues[1].depthStencil = { 1.0f, 0 };

			VkRenderPassBeginInfo renderPassBeginInfo = {};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.pNext = nullptr;
			renderPassBeginInfo.renderPass = target->GetVkRenderPass();
			renderPassBeginInfo.renderArea.offset.x = 0;
			renderPassBeginInfo.renderArea.offset.y = 0;
			renderPassBeginInfo.renderArea.extent.width = width;
			renderPassBeginInfo.renderArea.extent.height = height;
			renderPassBeginInfo.clearValueCount = 2;
			renderPassBeginInfo.pClearValues = clearValues;
			// Set target frame buffer
			renderPassBeginInfo.framebuffer = target->GetCurrentFramebuffer();

			// Start the first sub pass specified in our default render pass setup by the base class
			// This will clear the color and depth attachment
			vkCmdBeginRenderPass(cmd, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			// Update dynamic viewport state
			VkViewport viewport = {};
			viewport.x = 0;
			viewport.y = (float)height;
			viewport.height = -(float)height;
			viewport.width = (float)width;
			viewport.minDepth = (float)0.0f;
			viewport.maxDepth = (float)1.0f;
			vkCmdSetViewport(cmd, 0, 1, &viewport);

			// Update dynamic scissor state
			VkRect2D scissor = {};
			scissor.extent.width = width;
			scissor.extent.height = height;
			scissor.offset.x = 0;
			scissor.offset.y = 0;
			vkCmdSetScissor(cmd, 0, 1, &scissor);

			// Draw Imgui
			ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
			vkCmdEndRenderPass(cmd);
		}
	}

	void ImGuiVulkanImpl::InitResources()
	{
		CommandBufferStorage cmdStorage{};
		VulkanCommandBuffer::CreateCommandBuffer(&cmdStorage);
		{
			ImGui_ImplVulkan_CreateFontsTexture(cmdStorage.Buffer);
		}
		VulkanCommandBuffer::ExecuteCommandBuffer(&cmdStorage);

		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}
}
#endif