#pragma once
#ifndef OPENGL_IMPL
#include "GraphicsContext.h"

#include "Backends/Vulkan/VulkanAllocator.h"
#include "Backends/Vulkan/VulkanInstance.h"
#include "Backends/Vulkan/VulkanDevice.h"
#include "Backends/Vulkan/VulkanSwapchain.h"
#include "Backends/Vulkan/VulkanCommandBuffer.h"
#include "Backends/Vulkan/VulkanSemaphore.h"

#include "Backends/Vulkan/GUI/ImGuiVulkanImpl.h"
#include "Backends/Vulkan/GUI/NuklearVulkanImpl.h"

#ifdef AFTERMATH
#include "Backends/Vulkan/Aftermath/GpuCrashTracker.h"
#endif

struct GLFWwindow;

namespace SmolEngine
{
	class GraphicsContext;

	class VulkanContext: public GraphicsContext
	{
	public:
		VulkanContext() = default;
		~VulkanContext();

		virtual void                        SwapBuffersEX() override;
		virtual void                        BeginFrameEX(float time) override;
		virtual void                        ShutdownEX() override;
		virtual void                        ResizeEX(uint32_t* width, uint32_t* height) override;
		virtual void                        CreateAPIContextEX() override;
		virtual void                        OnEventEX(Event& event) override;
		virtual void                        OnContexReadyEX() override;

		// Getters
		inline static VulkanContext*        GetSingleton() { return dynamic_cast<VulkanContext*>(GraphicsContext::GetSingleton()); }
		inline static VulkanCommandBuffer&  GetCommandBuffer() { return m_CommandBuffer; }
		inline static VulkanSwapchain&      GetSwapchain() { return m_Swapchain; }
		inline static VulkanInstance&       GetInstance() { return m_Instance; }
		inline static VulkanDevice&         GetDevice() { return m_Device; }
		inline static VkCommandBuffer       GetCurrentVkCmdBuffer() { return m_CurrentVkCmdBuffer; }
		inline static uint64_t              GetBufferDeviceAddress(VkBuffer buffer);

#ifdef AFTERMATH
		inline static GpuCrashTracker&      GetCrashTracker() { return m_CrachTracker; }
#endif

	private:
		Ref<NuklearVulkanImpl>              m_NuklearContext = nullptr;
		Ref<ImGuiVulkanImpl>                m_ImGuiContext = nullptr;
		VulkanAllocator*                    m_Allocator = nullptr;
		inline static VkCommandBuffer       m_CurrentVkCmdBuffer = nullptr;
		inline static VulkanCommandBuffer   m_CommandBuffer = {};
		inline static VulkanSwapchain       m_Swapchain = {};
		inline static VulkanSemaphore       m_Semaphore = {};
		inline static VulkanInstance        m_Instance = {};
		inline static VulkanDevice          m_Device = {};
#ifdef AFTERMATH
		inline static GpuCrashTracker      m_CrachTracker{};
#endif

	private:

		friend class GraphicsContext;
		friend class ImGuiVulkanImpl;
	};
}
#endif
