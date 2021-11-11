#pragma once
#include "Graphics/GUI/UILayout.h"
#include "Graphics/GUI/UIStyle.h"
#include "Graphics/Primitives/Texture.h"

#include <functional>
#include <string>
#include <glm/glm.hpp>

namespace SmolEngine
{
	struct nk_style_button;

	class UIButton: public UILayout
	{
	public:
		UIButton();
		~UIButton();

		void                    Draw() override;

		UIStyle                 Style{};
		std::string             Label = "Button";
		std::function<void()>   OnPressed = nullptr;
		Texture*                NormalTex = nullptr;
		Texture*                PressTex = nullptr;
		Texture*                HoverTex = nullptr;

	private:
		nk_style_button*        pNkStyle = nullptr;
	};
}