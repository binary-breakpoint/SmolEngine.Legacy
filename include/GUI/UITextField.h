#pragma once
#include "GUI/UIStyle.h"
#include "GUI/UILayout.h"

#include <string>
#include <functional>

namespace SmolEngine
{
	enum class TextFieldFilter
	{
		filter_ascii,
		filter_binary,
		filter_decimal,
		filter_default,
		filter_float,
		filter_hex,
		filter_oct
	};

	class UITextField: public UILayout
	{
	public:
		UITextField();

		void             Draw() override;

		UIStyle          Style = {};
		std::string      Text = "";
		std::string      Label = "";
		TextFieldFilter  eFilter = TextFieldFilter::filter_default;
		std::function<void(const std::string&)> OnEnter = nullptr;

	private:
		int   Length = 0;
		char  Buffer[1024];
	};
}