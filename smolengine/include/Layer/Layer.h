#pragma once
#include "Core/Core.h"
#include "Window/Events.h"

#include <string>

namespace SmolEngine 
{
	class Layer
	{
	public:
		Layer(const std::string& name = "Default Layer");
		virtual ~Layer();

		virtual void OnAttach() {}
		virtual void OnDetach() {}
		virtual void OnBeginFrame(float deltaTime) {};
		virtual void OnEndFrame(float deltaTime) {};
		virtual void OnUpdate(float deltaTime) {}
		virtual void OnEvent(Event& event) {}
		virtual void OnImGuiRender() {}

		inline const std::string& GetName() const { return m_Name; }

	protected:
		std::string   m_Name;
		bool          m_Enabled;
	};
}

