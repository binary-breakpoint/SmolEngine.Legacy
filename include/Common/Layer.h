#pragma once
#include "Common/Core.h"
#include "Common/Time.h"
#include "Common/EventHandler.h"

#include <string>

namespace Frostium 
{
	class Layer
	{
	public:

		Layer(const std::string& name = "Basic Layer");

		virtual ~Layer();

		/// Overloads

		virtual void OnAttach() {}

		virtual void OnDetach() {}

		virtual void OnUpdate(DeltaTime deltaTime) {}

		virtual void OnEvent(Event& event) {}

		virtual void OnImGuiRender() {}

		/// Getters

		inline const std::string& GetName() const { return m_Name; }

	protected:

		bool         m_IsEnabled = true;
		std::string  m_Name;
	};
}

