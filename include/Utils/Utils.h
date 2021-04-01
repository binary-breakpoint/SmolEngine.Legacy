#pragma once
#include "Common/Core.h"

#include <glm/glm.hpp>

namespace Frostium
{
	enum class CachedPathType
	{
		Shader,
		Pipeline
	};

	class Utils
	{
	public:

		static bool DecomposeTransform(const glm::mat4& transform, glm::vec3& out_translation, glm::vec3& out_rotation, glm::vec3& out_scale);

		static bool ComposeTransform(const glm::vec3& translation, const glm::vec3& rotation, const glm::vec3& scale, bool is3D, glm::mat4& out_transform);

		static bool IsPathValid(const std::string& path);

		static std::string GetCachedPath(const std::string& filePath, CachedPathType type);
	};
}