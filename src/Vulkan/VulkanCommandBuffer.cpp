#include "stdafx.h"
#ifndef FROSTIUM_OPENGL_IMPL
#include "Vulkan/VulkanCommandBuffer.h"

#include "Vulkan/VulkanDevice.h"
#include "Vulkan/VulkanSwapchain.h"
#include "Vulkan/VulkanContext.h"

namespace Frostium
{
	VulkanCommandBuffer::VulkanCommandBuffer()
	{

	}

	VulkanCommandBuffer::~VulkanCommandBuffer()
	{

	}

	bool VulkanCommandBuffer::Init(VulkanDevice* device, VulkanSwapchain* swapchain)
	{
		m_Device = device;
		m_Swapchain = swapchain;
		return Create();
	}

	bool VulkanCommandBuffer::Create()
	{
		VkDevice device = m_Device->GetLogicalDevice();

		if (m_CommandPool == VK_NULL_HANDLE)
		{
			VkCommandPoolCreateInfo poolInfo = {};
			{
				poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
				poolInfo.queueFamilyIndex = m_Device->GetQueueFamilyIndex();
				poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
				VK_CHECK_RESULT(vkCreateCommandPool(device, &poolInfo, nullptr, &m_CommandPool));
			}
		}

		{
			uint32_t count = m_Swapchain->m_ImageCount;
			m_CommandBuffers.resize(count);

			VkCommandBufferAllocateInfo commandBufferInfo = {};
			{
				commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				commandBufferInfo.commandPool = m_CommandPool;
				commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				commandBufferInfo.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size());
			}

			VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &commandBufferInfo, m_CommandBuffers.data()));
		}


		return true;
	}

	bool VulkanCommandBuffer::Recrate()
	{
		if (!m_Device) { return false; }

		vkResetCommandPool(m_Device->GetLogicalDevice(), m_CommandPool, 0);
		return true;
	}

	VkCommandBuffer VulkanCommandBuffer::GetVkCommandBuffer() const
	{
		return m_CommandBuffers[m_Swapchain->GetCurrentBufferIndex()];
	}

	VkCommandPool VulkanCommandBuffer::GetVkCommandPool() const
	{
		return m_CommandPool;
	}

	const VkCommandBuffer VulkanCommandBuffer::CreateSingleCommandBuffer(bool oneCommand)
	{
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = VulkanContext::GetCommandBuffer().m_CommandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		VK_CHECK_RESULT(vkAllocateCommandBuffers(VulkanContext::GetDevice().GetLogicalDevice(), &allocInfo, &commandBuffer));

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		if (oneCommand)
		{
			beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		}

		VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &beginInfo));
		return commandBuffer;
	}

	void VulkanCommandBuffer::EndSingleCommandBuffer(const VkCommandBuffer cmdBuffer)
	{
		vkEndCommandBuffer(cmdBuffer);

		VkSubmitInfo submitInfo = {};
		{
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &cmdBuffer;
		}

		VkQueue q = VulkanContext::GetDevice().GetQueue();
		vkQueueSubmit(q, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(q);
		vkFreeCommandBuffers(VulkanContext::GetDevice().GetLogicalDevice(), VulkanContext::GetCommandBuffer().m_CommandPool, 1, &cmdBuffer);
	}

	void VulkanCommandBuffer::FlushCommandBuffer(const VkCommandBuffer cmdBuffer)
	{
		const uint64_t time_out = 100000000000;
		assert(cmdBuffer != VK_NULL_HANDLE);

		VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));
		VkSubmitInfo submitInfo = {};
		{
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &cmdBuffer;
		}

		VkFenceCreateInfo fenceCI = {};
		{
			fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceCI.flags = 0;
		}

		VkFence fence;
		VK_CHECK_RESULT(vkCreateFence(VulkanContext::GetDevice().GetLogicalDevice(), &fenceCI, nullptr, &fence));
		VK_CHECK_RESULT(vkQueueSubmit(VulkanContext::GetDevice().GetQueue(), 1, &submitInfo, fence));
		VK_CHECK_RESULT(vkWaitForFences(VulkanContext::GetDevice().GetLogicalDevice(), 1, &fence, VK_TRUE, time_out));

		vkDestroyFence(VulkanContext::GetDevice().GetLogicalDevice(), fence, nullptr);
		vkFreeCommandBuffers(VulkanContext::GetDevice().GetLogicalDevice(), VulkanContext::GetCommandBuffer().m_CommandPool, 1, &cmdBuffer);
	}

	size_t VulkanCommandBuffer::GetBufferSize() const
	{
		return m_CommandBuffers.size();
	}
}
#endif