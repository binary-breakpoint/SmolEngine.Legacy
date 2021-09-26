#include "stdafx.h"
#include "GUI/UITextField.h"
#include "GUI/Backends/NuklearContext.h"

namespace SmolEngine
{
#include "nuklear/nuklear.h"

	UITextField::UITextField()
	{
		Style.Text_Active = glm::vec4(90.0f, 90.0f, 90.0f, 255.0f);
		Style.Text_Hover = glm::vec4(90.0f, 90.0f, 90.0f, 255.0f);
		Style.Text_Normal = glm::vec4(90.0f, 90.0f, 90.0f, 255.0f);
	}

	void UITextField::Draw()
	{
		OnDraw();

		nk_color set_color;
		NuklearContext::GetColor(Style.BG_Active, &set_color);
		nk_style_push_color(pContext, &pContext->style.edit.active.data.color, set_color);

		NuklearContext::GetColor(Style.BG_Hover, &set_color);
		nk_style_push_color(pContext, &pContext->style.edit.hover.data.color, set_color);

		NuklearContext::GetColor(Style.BG_Normal, &set_color);
		nk_style_push_color(pContext, &pContext->style.edit.normal.data.color, set_color);

		NuklearContext::GetColor(Style.Border_Normal, &set_color);
		nk_style_push_color(pContext, &pContext->style.edit.border_color, set_color);

		NuklearContext::GetColor(Style.Text_Normal, &set_color);
		nk_style_push_color(pContext, &pContext->style.edit.text_normal, set_color);

		if (!Label.empty())
		{
			nk_flags alignment = 0;
			NuklearContext::GetTextAlignment(eAlignment, alignment);
			NuklearContext::GetColor(Style.Text_Label, &set_color);
			nk_label_colored_wrap(pContext, Label.c_str(), set_color);
		}

		nk_flags flag = 0;
		switch (eFilter)
		{
		case TextFieldFilter::filter_ascii: flag = nk_edit_string(pContext, NK_EDIT_FIELD, Buffer, &Length, 1024, nk_filter_ascii); break;
		case TextFieldFilter::filter_binary: flag = nk_edit_string(pContext, NK_EDIT_FIELD, Buffer, &Length, 1024, nk_filter_binary); break;
		case TextFieldFilter::filter_decimal: flag = nk_edit_string(pContext, NK_EDIT_FIELD, Buffer, &Length, 1024, nk_filter_decimal); break;
		case TextFieldFilter::filter_default: flag = nk_edit_string(pContext, NK_EDIT_FIELD, Buffer, &Length, 1024, nk_filter_default); break;
		case TextFieldFilter::filter_float: flag = nk_edit_string(pContext, NK_EDIT_FIELD, Buffer, &Length, 1024, nk_filter_float); break;
		case TextFieldFilter::filter_hex: flag = nk_edit_string(pContext, NK_EDIT_FIELD, Buffer, &Length, 1024, nk_filter_hex); break;
		case TextFieldFilter::filter_oct: flag = nk_edit_string(pContext, NK_EDIT_FIELD, Buffer, &Length, 1024, nk_filter_oct); break;
		}

		switch (flag)
		{
		case NK_EDIT_COMMITED:
		{
			Text = std::string(Buffer, Length);
			if (OnEnter != nullptr)
				OnEnter(Text);

			break;
		}
		}

		nk_style_pop_color(pContext);
		nk_style_pop_color(pContext);
		nk_style_pop_color(pContext);
		nk_style_pop_color(pContext);
		nk_style_pop_color(pContext);
	}
}