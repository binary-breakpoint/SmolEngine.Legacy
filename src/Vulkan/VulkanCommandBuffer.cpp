#include "stdafx.h"
#ifndef FROSTIUM_OPENGL_IMPL
#include "Vulkan/VulkanCommandBuffer.h"

#include "Vulkan/VulkanDevice.h"
#include "Vulkan/VulkanSwapchain.h"
#include "Vulkan/VulkanContext.h"

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
#ifdef FROSTIUM_SMOLENGINE_IMPL
	std::mutex* s_Mutex = nullptr;
#endif

	VulkanCommandBuffer::VulkanCommandBuffer()
	{
#ifdef FROSTIUM_SMOLENGINE_IMPL
		s_Mutex = new std::mutex();
#endif
	}

	bool VulkanCommandBuffer::Init(VulkanDevice* device)
	{
		m_Device = device;

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
		return m_CommandBuffers[VulkanContext::GetSwapchain().GetCurrentBufferIndex()];
	}

	VkCommandPool VulkanCommandBuffer::GetVkCommandPool() const
	{
		return m_CommandPool;
	}

	void VulkanCommandBuffer::CreateCommandBuffer(CommandBufferStorage* data)
	{
		if (data)
		{
			VkDevice device = VulkanContext::GetDevice().GetLogicalDevice();

#ifdef FROSTIUM_SMOLENGINE_IMPL
			VkCommandPoolCreateInfo poolInfo = {};
			{
				poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
				poolInfo.queueFamilyIndex = VulkanContext::GetDevice().GetQueueFamilyIndex();
				poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
				VK_CHECK_RESULT(vkCreateCommandPool(device, &poolInfo, nullptr, &data->Pool));
			}
#else
			data->Pool = VulkanContext::GetCommandBuffer().m_CommandPool;
#endif

			VkCommandBufferAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocInfo.commandPool = data->Pool;
			allocInfo.commandBufferCount = 1;
			VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &allocInfo, &data->Buffer));

			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

			VK_CHECK_RESULT(vkBeginCommandBuffer(data->Buffer, &beginInfo));

		}
	}

	void VulkanCommandBuffer::ExecuteCommandBuffer(CommandBufferStorage* data)
	{
		if (data)
		{
			VkDevice device = VulkanContext::GetDevice().GetLogicalDevice();
			const uint64_t time_out = 100000000000;
			VK_CHECK_RESULT(vkEndCommandBuffer(data->Buffer));

			VkSubmitInfo submitInfo = {};
			{
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submitInfo.commandBufferCount = 1;
				submitInfo.pCommandBuffers = &data->Buffer;
			}

			VkFence fence = {};
			VkFenceCreateInfo fenceCI = {};
			{
				fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
				fenceCI.flags = 0;

				VK_CHECK_RESULT(vkCreateFence(device, &fenceCI, nullptr, &fence));
			}

#ifdef FROSTIUM_SMOLENGINE_IMPL
			{
				const std::lock_guard<std::mutex> lock(*s_Mutex);
				VK_CHECK_RESULT(vkQueueSubmit(VulkanContext::GetDevice().GetQueue(), 1, &submitInfo, fence));
			}

			VK_CHECK_RESULT(vkWaitForFences(device, 1, &fence, VK_TRUE, time_out));
			vkDestroyFence(device, fence, nullptr);
			vkFreeCommandBuffers(device, data->Pool, 1, &data->Buffer);
			vkDestroyCommandPool(device, data->Pool, nullptr);
#else
			VK_CHECK_RESULT(vkQueueSubmit(VulkanContext::GetDevice().GetQueue(), 1, &submitInfo, fence));
			VK_CHECK_RESULT(vkWaitForFences(device, 1, &fence, VK_TRUE, time_out));
			vkDestroyFence(device, fence, nullptr);
			vkFreeCommandBuffers(device, data->Pool, 1, &data->Buffer);
#endif
		}
	}

	size_t VulkanCommandBuffer::GetBufferSize() const
	{
		return m_CommandBuffers.size();
	}
}
#endif