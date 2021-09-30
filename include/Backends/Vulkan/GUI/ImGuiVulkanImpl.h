#pragma once
#ifndef OPENGL_IMPL
#include "GUI/Backends/ContextBaseGUI.h"
#include "Backends/Vulkan/VulkanTexture.h"

#include <imgui/examples/imgui_impl_vulkan.h>

struct GLFWwindow;

namespace SmolEngine
{
	class VulkanCommandBuffer;

	class ImGuiVulkanImpl: public ContextBaseGUI
	{
	public:
		void               Init() override;
		void               ShutDown() override;
		void               NewFrame() override;
		void               EndFrame() override;
		void               OnEvent(Event& e) override;
		void               Draw(VulkanSwapchain* target) override;

	private:
		void               OnSetup();

	private:
		VkDevice           g_Device = VK_NULL_HANDLE;
		VkPipelineCache    g_PipelineCache = VK_NULL_HANDLE;
		VkDescriptorPool   g_DescriptorPool = VK_NULL_HANDLE;
	};
}

#endif