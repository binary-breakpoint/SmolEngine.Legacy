#include "stdafx.h"
#include "GUI/UICheckbox.h"

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
#include "nuklear/nuklear.h"

	void UICheckbox::Draw()
	{
		OnDraw();
		if (nk_checkbox_label(pContext, Label.c_str(), &bActive))
		{
			if (OnPressed) { OnPressed(); }
			if (OnSetActive && bActive != 0) { OnSetActive(); }
			else if (OnSetInactive && bActive == 0) { OnSetInactive(); }
		}
	}
}