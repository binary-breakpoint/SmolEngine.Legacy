#pragma once

#include <glm/glm.hpp>

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	struct UIStyle
	{
		glm::vec4 BG_Active = glm::vec4(255.0f);
		glm::vec4 BG_Hover = glm::vec4(255.0f);
		glm::vec4 BG_Normal = glm::vec4(255.0f);
		glm::vec4 Text_Active = glm::vec4(255.0f);
		glm::vec4 Text_Hover = glm::vec4(255.0f);
		glm::vec4 Text_Normal = glm::vec4(255.0f);
		glm::vec4 Text_Label = glm::vec4(255.0f);
		glm::vec4 Border_Normal = glm::vec4(230.0f, 230.0f, 230.0f, 255.0f);

		float     Padding = 1.0f;
	};
}