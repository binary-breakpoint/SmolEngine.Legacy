#pragma once
#ifndef OPENGL_IMPL
#include "Graphics/GraphicsContext.h"

#include "Graphics/Backends/Vulkan/VulkanAllocator.h"
#include "Graphics/Backends/Vulkan/VulkanInstance.h"
#include "Graphics/Backends/Vulkan/VulkanDevice.h"
#include "Graphics/Backends/Vulkan/VulkanSwapchain.h"
#include "Graphics/Backends/Vulkan/VulkanCommandBuffer.h"
#include "Graphics/Backends/Vulkan/VulkanSemaphore.h"

#include "Graphics/Backends/Vulkan/GUI/ImGuiVulkanImpl.h"
#include "Graphics/Backends/Vulkan/GUI/NuklearVulkanImpl.h"

#ifdef SMOLENGINE_DEBUG
#include "Graphics/Backends/Vulkan/Aftermath/GpuCrashTracker.h"
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

#ifdef  SMOLENGINE_DEBUG
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
#ifdef  SMOLENGINE_DEBUG
		inline static GpuCrashTracker      m_CrachTracker{};
#endif

	private:

		friend class GraphicsContext;
		friend class ImGuiVulkanImpl;
	};
}
#endif
