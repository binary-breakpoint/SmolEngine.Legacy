#include "stdafx.h"
#ifndef FROSTIUM_OPENGL_IMPL
#include "Backends/Vulkan/VulkanCommandBuffer.h"

#include "Backends/Vulkan/VulkanDevice.h"
#include "Backends/Vulkan/VulkanSwapchain.h"
#include "Backends/Vulkan/VulkanContext.h"

namespace SmolEngine
{
	VulkanCommandBuffer::VulkanCommandBuffer()
	{
		m_Mutex = new std::mutex();
	}

	bool VulkanCommandBuffer::Init(VulkanDevice* device)
	{
		m_Device = device;
		return Create();
	}

	bool VulkanCommandBuffer::Create()
	{
		VkDevice device = m_Device->GetLogicalDevice();

		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = m_Device->GetQueueFamilyIndices().Graphics;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		VK_CHECK_RESULT(vkCreateCommandPool(device, &poolInfo, nullptr, &m_CommandPool));

		poolInfo.queueFamilyIndex = m_Device->GetQueueFamilyIndices().Compute;
		VK_CHECK_RESULT(vkCreateCommandPool(device, &poolInfo, nullptr, &m_ComputeCommandPool));

		uint32_t count = VulkanContext::GetSwapchain().m_ImageCount;
		m_CommandBuffers.resize(count);
		VkCommandBufferAllocateInfo commandBufferInfo = {};
		{
			commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			commandBufferInfo.commandPool = m_CommandPool;
			commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			commandBufferInfo.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size());
		}

		VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &commandBufferInfo, m_CommandBuffers.data()));

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
		return m_CommandBuffers[VulkanContext::GetSwapchain().GetCurrentBufferIndex()];
	}

	VkCommandPool VulkanCommandBuffer::GetVkCommandPool(bool compute) const
	{
		return compute ? m_ComputeCommandPool: m_CommandPool;
	}

	void VulkanCommandBuffer::CreateCommandBuffer(CommandBufferStorage* data)
	{
		VkDevice device = VulkanContext::GetDevice().GetLogicalDevice();
		if (data->bNewPool)
		{
			VkCommandPoolCreateInfo poolInfo = {};
			{
				auto& queueFamilyIndices = VulkanContext::GetDevice().GetQueueFamilyIndices();

				poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
				poolInfo.queueFamilyIndex = data->bCompute ? queueFamilyIndices.Compute: queueFamilyIndices.Graphics;
				poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
				VK_CHECK_RESULT(vkCreateCommandPool(device, &poolInfo, nullptr, &data->Pool));
			}
		}
		else
		{
			data->Pool = data->bCompute ? VulkanContext::GetCommandBuffer().m_ComputeCommandPool:
				VulkanContext::GetCommandBuffer().m_CommandPool;
		}

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = data->Pool;
		allocInfo.commandBufferCount = 1;
		VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &allocInfo, &data->Buffer));

		if (data->bStartRecord)
		{
			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			VK_CHECK_RESULT(vkBeginCommandBuffer(data->Buffer, &beginInfo));
		}
	}

	void VulkanCommandBuffer::ExecuteCommandBuffer(CommandBufferStorage* data)
	{
		constexpr uint64_t time_out = 100000000000;
		VkDevice device = VulkanContext::GetDevice().GetLogicalDevice();
		VK_CHECK_RESULT(vkEndCommandBuffer(data->Buffer));

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &data->Buffer;

		if (data->bCompute)
		{
			VkFenceCreateInfo fenceCreateInfo{};
			fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
			VK_CHECK_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &data->ComputeFence));

			// Make sure previous compute shader in pipeline has completed (TODO: this shouldn't be needed for all cases)
			vkWaitForFences(device, 1, &data->ComputeFence, VK_TRUE, UINT64_MAX);
			vkResetFences(device, 1, &data->ComputeFence);

			VK_CHECK_RESULT(vkQueueSubmit(VulkanContext::GetDevice().GetQueue(QueueFamilyFlags::Compute), 1, &submitInfo, data->ComputeFence));

			VK_CHECK_RESULT(vkWaitForFences(device, 1, &data->ComputeFence, VK_TRUE, UINT64_MAX));
		}
		else
		{
			VkFence fence = {};
			VkFenceCreateInfo fenceCI = {};
			fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceCI.flags = 0;
			VK_CHECK_RESULT(vkCreateFence(device, &fenceCI, nullptr, &fence));

			{
				std::lock_guard<std::mutex> lock(*m_Mutex);
				VK_CHECK_RESULT(vkQueueSubmit(VulkanContext::GetDevice().GetQueue(QueueFamilyFlags::Graphics), 1, &submitInfo, fence));
			}

			VK_CHECK_RESULT(vkWaitForFences(device, 1, &fence, VK_TRUE, time_out));
			vkDestroyFence(device, fence, nullptr);
		}

		vkFreeCommandBuffers(device, data->Pool, 1, &data->Buffer);
		if (data->bNewPool)
			vkDestroyCommandPool(device, data->Pool, nullptr);
	}

	size_t VulkanCommandBuffer::GetBufferSize() const
	{
		return m_CommandBuffers.size();
	}
}
#endif