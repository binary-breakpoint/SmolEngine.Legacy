#pragma once
#ifndef OPENGL_IMPL
#include "Backends/Vulkan/VulkanAllocator.h"
#include "Backends/Vulkan/VulkanInstance.h"
#include "Backends/Vulkan/VulkanDevice.h"
#include "Backends/Vulkan/VulkanSwapchain.h"
#include "Backends/Vulkan/VulkanCommandBuffer.h"
#include "Backends/Vulkan/VulkanSemaphore.h"

struct GLFWwindow;

namespace SmolEngine
{
	class GraphicsContext;

	class VulkanContext
	{
	public:
		VulkanContext() = default;
		~VulkanContext();

		void                                Setup(GLFWwindow* window, GraphicsContext* context, uint32_t* width, uint32_t* height);
		void                                Shutdown();
		void                                BeginFrame();
		void                                SwapBuffers(bool skip = false);
		void                                OnResize(uint32_t* width, uint32_t* height);
		// Getters
		inline static VulkanContext*        GetSingleton() { return s_ContextInstance; }
		inline GLFWwindow*                  GetWindow() { return m_Window; }
		inline static VulkanCommandBuffer&  GetCommandBuffer() { return m_CommandBuffer; }
		inline static VulkanSwapchain&      GetSwapchain() { return m_Swapchain; }
		inline static VulkanInstance&       GetInstance() { return m_Instance; }
		inline static VulkanDevice&         GetDevice() { return m_Device; }
		inline static VkCommandBuffer       GetCurrentVkCmdBuffer() { return m_CurrentVkCmdBuffer; }
		inline static uint64_t              GetBufferDeviceAddress(VkBuffer buffer);

	private:
		GLFWwindow*                         m_Window = nullptr;
		GraphicsContext*                    m_Context = nullptr;
		VulkanAllocator                     m_Allocator = {};
		inline static VulkanContext*        s_ContextInstance = nullptr;
		inline static VkCommandBuffer       m_CurrentVkCmdBuffer = nullptr;
		inline static VulkanCommandBuffer   m_CommandBuffer = {};
		inline static VulkanSwapchain       m_Swapchain = {};
		inline static VulkanSemaphore       m_Semaphore = {};
		inline static VulkanInstance        m_Instance = {};
		inline static VulkanDevice          m_Device = {};

	private:

		friend class GraphicsContext;
		friend class ImGuiVulkanImpl;
	};
}
#endif
