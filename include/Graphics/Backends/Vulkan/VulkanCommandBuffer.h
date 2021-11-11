#pragma once
#ifndef OPENGL_IMPL

#include "Graphics/Backends/Vulkan/Vulkan.h"

#include <mutex>

namespace SmolEngine
{
	class VulkanDevice;
	class VulkanCommandPool;

	struct CommandBufferStorage
	{
		VkCommandBuffer Buffer = VK_NULL_HANDLE;
		VkCommandPool   Pool = VK_NULL_HANDLE;
		VkFence         ComputeFence = VK_NULL_HANDLE;
		bool            bNewPool = true;
		bool            bCompute = false;
		bool            bStartRecord = true;
	};

	class VulkanCommandBuffer
	{
	public:
		VulkanCommandBuffer();

		bool                           Init(VulkanDevice* device);
		bool                           Create();
		bool                           Recrate();

		static void                    CreateCommandBuffer(CommandBufferStorage* data);
		static void                    ExecuteCommandBuffer(CommandBufferStorage* data);

		// Getters
		VkCommandBuffer                GetVkCommandBuffer() const;
		VkCommandPool                  GetVkCommandPool(bool compute = false) const;
		size_t                         GetBufferSize() const;

		inline static std::mutex*      m_Mutex = nullptr;

	private:
		VkCommandPool                  m_CommandPool = VK_NULL_HANDLE;
		VkCommandPool                  m_ComputeCommandPool = VK_NULL_HANDLE;
		VulkanDevice*                  m_Device = nullptr;
		std::vector<VkCommandBuffer>   m_CommandBuffers;
	};
}
#endif