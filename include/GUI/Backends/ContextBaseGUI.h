#pragma once

#include "Window/Events.h"

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	class Framebuffer;
	class VulkanSwapchain;

	class ContextBaseGUI
	{
	public:

		virtual void Init() {}
		virtual void ShutDown() {}

		virtual void NewFrame() {}
		virtual void EndFrame() {}

		virtual void Draw(Framebuffer* target) {}
		virtual void Draw(VulkanSwapchain* target) {}

		virtual void OnEvent(Event& e) {}
	};
}