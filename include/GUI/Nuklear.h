#pragma once

#include "GUI/UILayout.h"

namespace SmolEngine
{
	struct nk_color;

	class Nuklear
	{
	public:
		// Helpers
		static void    GetColor(const glm::vec4& col, nk_color* out);
		static void    GetTextAlignment(AlignmentFlags flag, uint32_t& out);
	};
}