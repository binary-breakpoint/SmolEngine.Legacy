#pragma once
#include "GUI/Backends/ContextBaseGUI.h"
#include "GUI/UILayout.h"

#include <glm/glm.hpp>

namespace SmolEngine
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