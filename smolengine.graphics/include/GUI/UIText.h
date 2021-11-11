#pragma once
#include "GUI/UILayout.h"
#include "GUI/UIStyle.h"
#include "Primitives/Texture.h"

#include <glm/glm.hpp>

namespace SmolEngine
{
	struct nk_font;
	struct nk_font_atlas;

	enum class TextFilter
	{

	};

	class UIText: public UILayout
	{
	public:
		UIText() = default;
		~UIText();

		void            Draw() override;
		void            SetFont(const std::string& path, float size);
		void            SetSize(float size);
		void            SetPadding(float padding);

		UIStyle         Style{};
		float           Size = 14.0f;
		std::string     Text = "";

	private:
		void            Free();

	private:
		std::string     FontPath = "";
		nk_font*        pUserFont = nullptr;
	};				   
}