#include "stdafx.h"
#include "GUI/UIButton.h"

#include "GUI/Backends/NuklearContext.h"

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
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
        NuklearContext::GetColor(Style.BG_Hover, &pNkStyle->hover.data.color);
        NuklearContext::GetColor(Style.BG_Active, &pNkStyle->active.data.color);
        NuklearContext::GetColor(Style.BG_Normal, &pNkStyle->normal.data.color);
        NuklearContext::GetColor(Style.Border_Normal, &pNkStyle->border_color);
        NuklearContext::GetColor(Style.Text_Normal, &pNkStyle->text_normal);
        NuklearContext::GetTextAlignment(eAlignment, pNkStyle->text_alignment);

        OnDraw();

        if (nk_button_label_styled(pContext, pNkStyle, Label.c_str()) && OnPressed != nullptr)
        {
            OnPressed();
        }
    }
}