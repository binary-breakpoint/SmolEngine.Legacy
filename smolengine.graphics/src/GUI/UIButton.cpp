#include "stdafx.h"
#include "GUI/UIButton.h"
#include "GUI/Nuklear.h"

namespace SmolEngine
{
#include "nuklear/nuklear.h"

    UIButton::UIButton()
    {
        pNkStyle = new nk_style_button();
        *pNkStyle = pContext->style.button;

        Style.BG_Active = glm::vec4(180.0f, 220.0f, 170.0f, 255.0f);
        Style.BG_Hover = glm::vec4(255.0f);
        Style.Text_Normal = glm::vec4(220.0f, 220.0f, 220.0f, 255.0f);
        Style.Text_Normal = glm::vec4(120.0f, 120.0f, 120.0f, 255.0f);
    }

    UIButton::~UIButton()
    {
        delete pNkStyle;
    }

    void UIButton::Draw()
    {
        Nuklear::GetColor(Style.BG_Hover, &pNkStyle->hover.data.color);
        Nuklear::GetColor(Style.BG_Active, &pNkStyle->active.data.color);
        Nuklear::GetColor(Style.BG_Normal, &pNkStyle->normal.data.color);
        Nuklear::GetColor(Style.Border_Normal, &pNkStyle->border_color);
        Nuklear::GetColor(Style.Text_Normal, &pNkStyle->text_normal);
        Nuklear::GetTextAlignment(eAlignment, pNkStyle->text_alignment);

        pNkStyle->rounding = Style.Rounding;

        OnDraw();

        if (nk_button_label_styled(pContext, pNkStyle, Label.c_str()) && OnPressed != nullptr)
        {
            OnPressed();
        }
    }
}