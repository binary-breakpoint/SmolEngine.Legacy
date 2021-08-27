#pragma once
#include "GUI/Backends/ContextBaseGUI.h"

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	class NuklearContext
	{
	public:
		static ContextBaseGUI* CreateContext();
	};
}