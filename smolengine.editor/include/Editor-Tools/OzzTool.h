#pragma once

#include <string>

namespace SmolEngine
{
	class OzzTool
	{
	public:
		void Update(bool& open);

	private:
		std::string m_Path = "";
	};
}