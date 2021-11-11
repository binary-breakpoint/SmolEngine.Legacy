#pragma once
#ifndef OPENGL_IMPL
#include "Backends/Vulkan/VulkanTexture.h"

#include <imgui/examples/imgui_impl_vulkan.h>

struct GLFWwindow;

namespace SmolEngine
{
	class VulkanCommandBuffer;
	class VulkanSwapchain;

	class ImGuiVulkanImpl
	{
	public:
		void               Init();
		void               ShutDown();
		void               NewFrame();
		void               EndFrame();
		void               OnEvent(Event& e);
		void               Draw(VulkanSwapchain* target);

	private:
		void               OnSetup();

	private:
		VkDevice           g_Device = VK_NULL_HANDLE;
		VkPipelineCache    g_PipelineCache = VK_NULL_HANDLE;
		VkDescriptorPool   g_DescriptorPool = VK_NULL_HANDLE;
	};
}

#endif