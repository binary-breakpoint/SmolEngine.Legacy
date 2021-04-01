#pragma once

#include "Vulkan/Vulkan.h"
#include <imgui/examples/imgui_impl_vulkan.h>

struct GLFWwindow;

namespace Frostium
{
	class VulkanCommandBuffer;

	class ImGuiVulkanImpl
	{
	public:

		void Init();

		void Reset();

		void InitResources();

	private:

		VkDevice         g_Device;
		VkPipelineCache  g_PipelineCache = VK_NULL_HANDLE;
		VkDescriptorPool g_DescriptorPool = VK_NULL_HANDLE;

		friend class ImGuiLayer;
	};
}