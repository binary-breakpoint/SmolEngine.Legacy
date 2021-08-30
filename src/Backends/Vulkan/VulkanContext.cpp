#include "stdafx.h"
#ifndef FROSTIUM_OPENGL_IMPL
#include "Backends/Vulkan/VulkanContext.h"
#include "Backends/Vulkan/VulkanRenderPass.h"

#include "Common/Common.h"

#include "GUI/Backends/ImGuiContext.h"
#include "GUI/Backends/NuklearContext.h"

#include <GLFW/glfw3.h>
#include <imgui/examples/imgui_impl_vulkan.h>

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	VulkanContext::~VulkanContext()
	{

	}

	void VulkanContext::OnResize(uint32_t* width, uint32_t* height)
	{
		m_Swapchain.OnResize(width, height, m_Context->m_CreateInfo.bVsync, &m_CommandBuffer);
	}

	void VulkanContext::Setup(GLFWwindow* window, GraphicsContext* context, uint32_t* width, uint32_t* height)
	{
		assert(glfwVulkanSupported() == GLFW_TRUE);
		bool swapchain_initialized = false;
		{
			m_Instance.Init();
			m_Device.Init(&m_Instance);
			swapchain_initialized = m_Swapchain.Init(&m_Instance, &m_Device, window, context->m_CreateInfo.bTargetsSwapchain ? false: true);
			if (swapchain_initialized)
			{
				m_Swapchain.Create(width, height, context->m_CreateInfo.bVsync);
				m_CommandBuffer.Init(&m_Device);
				m_Semaphore.Init(&m_Device, &m_CommandBuffer);
				m_Swapchain.Prepare(*width, *height);
			}
		}

		if (swapchain_initialized)
		{
			s_ContextInstance = this;
			m_Window = window;
			m_Context = context;
			return;
		}

		DebugLog::LogError("Couldn't create Vulkan context!");
		abort();
	}

	void VulkanContext::BeginFrame()
	{
		// Get next image in the swap chain (back/front buffer)
		VK_CHECK_RESULT(m_Swapchain.AcquireNextImage(m_Semaphore.GetPresentCompleteSemaphore()));

		m_CurrentVkCmdBuffer = VulkanContext::GetCommandBuffer().GetVkCommandBuffer();
		VkCommandBufferBeginInfo cmdBufInfo = {};
		{
			cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdBufInfo.pNext = nullptr;
		}

		VK_CHECK_RESULT(vkBeginCommandBuffer(m_CurrentVkCmdBuffer, &cmdBufInfo));
	}

	void VulkanContext::SwapBuffers(bool skip)
	{
		// ImGUI pass
		if ((m_Context->m_CreateInfo.eFeaturesFlags & FeaturesFlags::Imgui) == FeaturesFlags::Imgui)
			m_Context->m_ImGuiContext->Draw(&m_Swapchain);

		// Nuklear pass
		m_Context->m_NuklearContext->Draw(m_Context->m_Framebuffer);

		const auto& present_ref = m_Semaphore.GetPresentCompleteSemaphore();
		const auto& render_ref = m_Semaphore.GetRenderCompleteSemaphore();
		constexpr uint64_t DEFAULT_FENCE_TIME_OUT = 100000000000;
		// Use a fence to wait until the command buffer has finished execution before using it again
		VK_CHECK_RESULT(vkWaitForFences(m_Device.GetLogicalDevice(), 1, &m_Semaphore.GetVkFences()[m_Swapchain.GetCurrentBufferIndex()], VK_TRUE, UINT64_MAX));
		VK_CHECK_RESULT(vkResetFences(m_Device.GetLogicalDevice(), 1, &m_Semaphore.GetVkFences()[m_Swapchain.GetCurrentBufferIndex()]));
		// Pipeline stage at which the queue submission will wait (via pWaitSemaphores)
		VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		// The submit info structure specifies a command buffer queue submission batch
		VkSubmitInfo submitInfo = {};
		{
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.pWaitDstStageMask = &waitStageMask;               // Pointer to the list of pipeline stages that the semaphore waits will occur at
			submitInfo.pWaitSemaphores = &present_ref;      // Semaphore(s) to wait upon before the submitted command buffer starts executing
			submitInfo.waitSemaphoreCount = 1;                           // One wait semaphore
			submitInfo.pSignalSemaphores = &render_ref;     // Semaphore(s) to be signaled when command buffers have completed
			submitInfo.signalSemaphoreCount = 1;                         // One signal semaphore
			submitInfo.pCommandBuffers = &m_CurrentVkCmdBuffer; // Command buffers(s) to execute in this batch (submission)
			submitInfo.commandBufferCount = 1;                           // One command buffer
		}

		// VK_IMAGE_LAYOUT_PRESENT_SRC_KHR for presenting it to the windowing system
		VK_CHECK_RESULT(vkEndCommandBuffer(m_CurrentVkCmdBuffer));

		// Submit to the graphics queue passing a wait fence
#ifdef FROSTIUM_SMOLENGINE_IMPL
		VulkanCommandBuffer::m_Mutex->lock();
		VK_CHECK_RESULT(vkQueueSubmit(m_Device.GetQueue(QueueFamilyFlags::Graphics), 1, &submitInfo, m_Semaphore.GetVkFences()[m_Swapchain.GetCurrentBufferIndex()]));
		VulkanCommandBuffer::m_Mutex->unlock();
#else
		VK_CHECK_RESULT(vkQueueSubmit(m_Device.GetQueue(), 1, &submitInfo, m_Semaphore.GetVkFences()[m_Swapchain.GetCurrentBufferIndex()]));
#endif
		// Present the current buffer to the swap chain
		// Pass the semaphore signaled by the command buffer submission from the submit info as the wait semaphore for swap chain presentation
		// This ensures that the image is not presented to the windowing system until all commands have been submitted
		VkResult present = m_Swapchain.QueuePresent(m_Device.GetQueue(QueueFamilyFlags::Graphics), render_ref);
		if (!((present == VK_SUCCESS) || (present == VK_SUBOPTIMAL_KHR))) {

			if (present == VK_ERROR_OUT_OF_DATE_KHR)
			{
				uint32_t w = m_Swapchain.GetWidth();
				uint32_t h = m_Swapchain.GetHeight();

				m_Swapchain.OnResize(&w, &h, &m_CommandBuffer);
				return;
			}
			else
			{
				VK_CHECK_RESULT(present);
			}
		}

		VK_CHECK_RESULT(vkWaitForFences(m_Device.GetLogicalDevice(), 1, &m_Semaphore.GetVkFences()[m_Swapchain.GetCurrentBufferIndex()], VK_TRUE, DEFAULT_FENCE_TIME_OUT));
	}

	inline uint64_t VulkanContext::GetBufferDeviceAddress(VkBuffer buffer)
	{
		VkBufferDeviceAddressInfoKHR bufferDeviceAI{};
		bufferDeviceAI.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		bufferDeviceAI.buffer = buffer;
		return vkGetBufferDeviceAddressKHR(m_Device.GetLogicalDevice(), &bufferDeviceAI);
	}
}
#endif