#pragma once
#include "GUI/UILayout.h"

#include <functional>
#include <string>
#include <glm/glm.hpp>

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	class UICheckbox: public UILayout
	{
	public:
		void                  Draw() override;

		std::function<void()> OnPressed = nullptr;
		std::function<void()> OnSetActive = nullptr;
		std::function<void()> OnSetInactive = nullptr;
		std::string           Label = "";

	private:
		int                   bActive = 0;
	};
}