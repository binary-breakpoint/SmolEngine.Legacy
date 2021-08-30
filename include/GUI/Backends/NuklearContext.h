#pragma once
#include "GUI/Backends/ContextBaseGUI.h"
#include "GUI/UILayout.h"

#include <glm/glm.hpp>

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	struct nk_color;

	class NuklearContext
	{
	public:
		static ContextBaseGUI*   CreateContext();

		// Helpers
		static void              GetColor(const glm::vec4& col, nk_color* out);
		static void              GetTextAlignment(AlignmentFlags flag, uint32_t& out);
	};
}