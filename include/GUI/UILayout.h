#pragma once

#include <glm/glm.hpp>

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	struct nk_context;

	enum class AlignmentFlags
	{
		Top,
		Midle,
		Button,
		Left,
		Centered,
		Right,

		MidLeft,
		MidRight,
		MidCentered
	};

	enum class LayoutFlags
	{
		Static,
		Dynamic
	};

	class UILayout
	{
	public:
		UILayout();

		virtual void      Draw() = 0;
		void              SetParent(UILayout* parent);
		void              SetAlignment(AlignmentFlags flag);
		void              SetLayoutFlag(LayoutFlags flag);
		void              SetLayout(float width, float height, int rows);
		void              SetOffset(float width, float height);

	private:
		void              OnDraw();

	private:
		UILayout*         pParent = nullptr;
		nk_context*       pContext = nullptr;
		LayoutFlags       eLayout = LayoutFlags::Static;
		AlignmentFlags    eAlignment = AlignmentFlags::MidCentered;
		glm::vec2         Size = glm::vec2(30.0f, 80.0f);
		glm::vec2         Offset = glm::vec2(0, 0);
		int               Rows = 1;

		friend class UIButton;
		friend class UIText;
		friend class UICheckbox;
		friend class UITextField;
	};
}