#pragma once
#include "Common/Core.h"

#include "Vulkan/VulkanInstance.h"
#include "Vulkan/VulkanDevice.h"
#include "Vulkan/VulkanCommandPool.h"
#include "Vulkan/VulkanSwapchain.h"
#include "Vulkan/VulkanCommandBuffer.h"
#include "Vulkan/VulkanSemaphore.h"

struct GLFWwindow;

namespace Frostium
{
	class VulkanContext
	{
	public:

		VulkanContext() = default;

		~VulkanContext();

		/// Main
		
		void OnResize(uint32_t width, uint32_t height);

		void Setup(GLFWwindow* window, uint32_t* width, uint32_t* height);

		void BeginFrame();

		void SwapBuffers(bool skip = false);

		/// Getters

		inline static VulkanContext* GetSingleton() { return s_ContextInstance; }

		inline GLFWwindow* GetWindow() { return m_Window; }


		inline static VulkanCommandBuffer& GetCommandBuffer() { return m_CommandBuffer; }

		inline static VulkanCommandPool& GetCommandPool() { return m_CommandPool; }

		inline static VulkanSwapchain& GetSwapchain() { return m_Swapchain; }

		inline static VulkanInstance& GetInstance() { return m_Instance; }

		inline static VulkanDevice& GetDevice() { return m_Device; }

		static VkCommandBuffer GetCurrentVkCmdBuffer() { return m_CurrentVkCmdBuffer; }


	private:

		inline static VulkanContext*            s_ContextInstance = nullptr;
		inline static VkCommandBuffer           m_CurrentVkCmdBuffer = nullptr;
		inline static VulkanCommandBuffer       m_CommandBuffer = {};
		inline static VulkanCommandPool         m_CommandPool = {};
		inline static VulkanSwapchain           m_Swapchain = {};
		inline static VulkanSemaphore           m_Semaphore = {};
		inline static VulkanInstance            m_Instance = {};
		inline static VulkanDevice              m_Device = {};
		GLFWwindow*                             m_Window = nullptr;

		bool                                    m_IsInitialized = false;


		friend class ImGuiVulkanImpl;
	};
}