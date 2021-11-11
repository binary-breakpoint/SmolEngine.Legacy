#include "stdafx.h"
#include "GUI/UILayout.h"

#include "Backends/Vulkan/GUI/NuklearVulkanImpl.h"

namespace SmolEngine
{
#include "nuklear/nuklear.h"

	UILayout::UILayout()
	{
		pContext = NuklearVulkanImpl::s_Instance->GetContext();
	}

	void UILayout::SetParent(UILayout* parent)
	{
		pParent = parent;
	}

	void UILayout::SetAlignment(AlignmentFlags flag)
	{
		eAlignment = flag;
	}

	void UILayout::SetLayoutFlag(LayoutFlags flag)
	{
		eLayout = flag;
	}

	void UILayout::SetLayout(float width, float height, int rows)
	{
		Rows = rows;
		Size.x = width;
		Size.y = height;
	}

	void UILayout::SetOffset(float width, float height)
	{
		Offset.x = width;
		Offset.y = height;
	}

	void UILayout::OnDraw()
	{
		if (pParent == nullptr)
		{
			switch (eLayout)
			{
			case LayoutFlags::Static: nk_layout_row_static(pContext, Size.y, static_cast<int>(Size.x), Rows); break;
			case LayoutFlags::Dynamic: nk_layout_row_dynamic(pContext, Size.y, Rows); break;
			}
		}
	}
}