#pragma once
#ifndef FROSTIUM_OPENGL_IMPL
#include "Common/Core.h"
#include "Common/Common.h"
#include "Vulkan/Vulkan.h"

namespace Frostium
{
	class VulkanDevice;
	class VulkanSwapchain;
	class VulkanCommandPool;

	class VulkanCommandBuffer
	{
	public:

		VulkanCommandBuffer();
		~VulkanCommandBuffer();

		bool                           Init(VulkanDevice* device, VulkanSwapchain* swapchain);
		bool                           Create();
		bool                           Recrate();

		static void                    EndSingleCommandBuffer(const VkCommandBuffer cmdBuffer);
		static void                    FlushCommandBuffer(const VkCommandBuffer cmdBuffer);
		static const VkCommandBuffer   CreateSingleCommandBuffer(bool oneCommand = true);

		// Getters
		VkCommandBuffer                GetVkCommandBuffer() const;
		VkCommandPool                  GetVkCommandPool() const;
		size_t                         GetBufferSize() const;

	private:

		VkCommandPool                  m_CommandPool = VK_NULL_HANDLE;
		VulkanSwapchain*               m_Swapchain = nullptr;
		VulkanDevice*                  m_Device = nullptr;
		std::vector<VkCommandBuffer>   m_CommandBuffers;
	};
}
#endif