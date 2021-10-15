#pragma once
#include "Common/BufferLayout.h"

#include <glm/glm.hpp>

namespace SmolEngine
{
	struct VertexInputInfo
	{
		VertexInputInfo() = default;
		VertexInputInfo(uint32_t stride, const BufferLayout& layout, bool isInputRateInstance = false)
			:
			Stride(stride),
			Layout(layout),
			IsInputRateInstance(isInputRateInstance) {}

		uint32_t          Stride = 0;
		BufferLayout      Layout{};
		bool              IsInputRateInstance = false;
	};

	struct PBRVertex
	{
		glm::vec3          Pos = glm::vec3(0.0f);
		glm::vec3          Normals = glm::vec3(0.0f);
		glm::vec3          Tangent = glm::vec3(0.0f);
		glm::vec2          UVs = glm::vec2(0.0f);
		glm::ivec4         jointIndices = glm::ivec4(0);
		glm::vec4          jointWeight = glm::vec4(0.0f);
	};

	struct TextVertex
	{
		glm::vec3 Pos;
		glm::vec2 UV;
	};
}