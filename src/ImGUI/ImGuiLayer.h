#pragma once

#include "Common/Layer.h"

#ifndef SMOLENIGNE_OPENGL_IMPL
#include <ImGUI/ImGuiVulkanImpl.h>
#endif

namespace Frostium 
{
	class Window;

	class ImGuiLayer: public Layer
	{
	public:

		ImGuiLayer();

		~ImGuiLayer();

		void OnEvent(Event& event) override;

		void OnAttach() override;

		void OnDetach() override;

		void OnImGuiRender() override;

		void OnBegin();

		void OnEnd();

	private:

#ifndef FROSTIUM_OPENGL_IMPL

		ImGuiVulkanImpl m_VulkanImpl = {};
#endif
	};

}
