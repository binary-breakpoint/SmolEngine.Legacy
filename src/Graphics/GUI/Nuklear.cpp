#include "stdafx.h"
#include "Graphics/GUI/Nuklear.h"

namespace SmolEngine
{
#include "nuklear/nuklear.h"

	void Nuklear::GetColor(const glm::vec4& col, nk_color* out)
	{
		*out = nk_rgba(static_cast<int>(col.r), static_cast<int>(col.g), static_cast<int>(col.b), static_cast<int>(col.a));
	}

	void Nuklear::GetTextAlignment(AlignmentFlags flag, uint32_t& out)
	{
		switch (flag)
		{
		case AlignmentFlags::Left: out = NK_TEXT_ALIGN_LEFT; break;
		case AlignmentFlags::Top: out = NK_TEXT_ALIGN_TOP; break;
		case AlignmentFlags::Button: out = NK_TEXT_ALIGN_BOTTOM; break;
		case AlignmentFlags::Centered: out = NK_TEXT_ALIGN_CENTERED; break;
		case AlignmentFlags::Right: out = NK_TEXT_ALIGN_RIGHT; break;
		case AlignmentFlags::MidCentered: out = NK_TEXT_CENTERED; break;
		case AlignmentFlags::MidLeft: out = NK_TEXT_LEFT; break;
		case AlignmentFlags::MidRight: out = NK_TEXT_RIGHT; break;
		}
	}
}