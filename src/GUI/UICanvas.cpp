#include "stdafx.h"
#include "GUI/UICanvas.h"

#include "GUI/Backends/NuklearContext.h"
#include "Backends/Vulkan/GUI/NuklearVulkanImpl.h"

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
#include "nuklear/nuklear.h"

    nk_flags GetFlagas(UICanvasFlags flags)
    {
        nk_flags nkFlags = 0;

        if ((flags & UICanvasFlags::NoInput) == UICanvasFlags::NoInput)
            nkFlags |= NK_WINDOW_NO_INPUT;

        if ((flags & UICanvasFlags::NoSrollbar) == UICanvasFlags::NoSrollbar)
            nkFlags |= NK_WINDOW_NO_SCROLLBAR;

        if ((flags & UICanvasFlags::Closable) == UICanvasFlags::Closable)
            nkFlags |= NK_WINDOW_CLOSABLE;

        if ((flags & UICanvasFlags::Minimizable) == UICanvasFlags::Minimizable)
            nkFlags |= NK_WINDOW_MINIMIZABLE;

        if ((flags & UICanvasFlags::Borders) == UICanvasFlags::Borders)
            nkFlags |= NK_WINDOW_BORDER;

        return nkFlags;
    }

    UICanvas::UICanvas()
    {
        pContext = NuklearVulkanImpl::s_Instance->GetContext();

        Style.BG_Normal = glm::vec4(0.0f);
        Style.Border_Normal = glm::vec4(0.0f);
    }

    void UICanvas::Draw(const std::function<void()>& func)
    {
        auto& nk_style = pContext->style.window;

        nk_color set_color;
        NuklearContext::GetColor(Style.BG_Normal, &set_color);

        nk_style_push_color(pContext, &nk_style.header.active.data.color, set_color);
        nk_style_push_color(pContext, &nk_style.header.hover.data.color, set_color);
        nk_style_push_color(pContext, &nk_style.header.normal.data.color, set_color);
        nk_style_push_color(pContext, &nk_style.fixed_background.data.color, set_color);

        NuklearContext::GetColor(Style.Border_Normal, &set_color);
        nk_style_push_color(pContext, &nk_style.border_color, set_color);

        if (nk_begin_titled(pContext, std::string(Title + "_##WindID").c_str(), Title.c_str(), nk_rect(Rect.x, Rect.y, Rect.z, Rect.w), GetFlagas(eFlags)))
        {
            func();
        }

        nk_style_pop_color(pContext);
        nk_style_pop_color(pContext);
        nk_style_pop_color(pContext);
        nk_style_pop_color(pContext);
        nk_style_pop_color(pContext);

        nk_end(pContext);
    }
}