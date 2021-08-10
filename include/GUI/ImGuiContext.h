#pragma once

#ifndef FROSTIUM_OPENGL_IMPL
#include <Backends/Vulkan/GUI/ImGuiVulkanImpl.h>
#endif
#include "Window/Events.h"

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	struct GraphicsContextInitInfo;

	class Window;
	class Framebuffer;
	class VulkanSwapchain;

	class ImGuiContext
	{
	public:

		void Init(GraphicsContextInitInfo* info);
		void ShutDown();
		void Draw(Framebuffer* target);
		void Draw(VulkanSwapchain* target);

		// Events
		void OnEvent(Event& event);
		void OnBegin();
		void OnEnd();

	private:
#ifndef FROSTIUM_OPENGL_IMPL
		ImGuiVulkanImpl m_VulkanImpl = {};
#endif
	};

}
