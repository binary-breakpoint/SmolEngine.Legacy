#include "stdafx.h"
#include "GUI/UIText.h"

#include "GUI/Backends/NuklearContext.h"
#include "Backends/Vulkan/GUI/NuklearVulkanImpl.h"

namespace SmolEngine
{
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#include "nuklear/nuklear.h"

    UIText::~UIText()
    {
        Free();
    }

    void UIText::Draw()
    {
        nk_color nk_color;
        nk_flags nk_alignment = 0;

        NuklearContext::GetColor(Style.Text_Normal, &nk_color);
        NuklearContext::GetTextAlignment(eAlignment, nk_alignment);

        OnDraw();

        if (pUserFont != nullptr) { nk_style_push_font(pContext, &pUserFont->handle); }
        nk_label_colored(pContext, Text.c_str(), nk_alignment, nk_color);
        if (pUserFont != nullptr) { nk_style_pop_font(pContext); }
    }

    void UIText::SetFont(const std::string& path, float size)
    {
        Free();

        FontPath = path;
        Size = size;
        auto atlas = NuklearVulkanImpl::s_Instance->GetFontAtlas();
        pUserFont = nk_font_atlas_add_from_file(atlas, path.c_str(), size, nullptr);

        NuklearVulkanImpl::s_Instance->UpdateAtlas();
    }

    void UIText::SetSize(float size)
    {
        if (pUserFont != nullptr)
        {
            Size = size;
            pUserFont->handle.height = size;
        }
    }

    void UIText::SetPadding(float padding)
    {
        if (pUserFont != nullptr)
        {
            Style.Padding = padding;
        }
    }

    void UIText::Free()
    {
        if (pUserFont != nullptr)
            delete pUserFont;
    }
}