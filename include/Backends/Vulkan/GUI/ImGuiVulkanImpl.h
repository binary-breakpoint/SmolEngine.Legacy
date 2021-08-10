#pragma once
#ifndef FROSTIUM_OPENGL_IMPL
#include "Backends/Vulkan/VulkanTexture.h"

#include <imgui/examples/imgui_impl_vulkan.h>

struct GLFWwindow;

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	class VulkanCommandBuffer;
	class Framebuffer;
	class VulkanSwapchain;

	class ImGuiVulkanImpl
	{
	public:

		void Init();
		void Reset();
		void Draw(Framebuffer* target);
		void Draw(VulkanSwapchain* target);
		void InitResources();

		VkDevice                                    g_Device;
		VkPipelineCache                             g_PipelineCache = VK_NULL_HANDLE;
		VkDescriptorPool                            g_DescriptorPool = VK_NULL_HANDLE;
	};
}

#endif