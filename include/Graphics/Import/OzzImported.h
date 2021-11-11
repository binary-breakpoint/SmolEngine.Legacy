#pragma once
#include <string>
#include <unordered_map>

namespace SmolEngine
{
	enum class OzzPath
	{
		Model,
		Skeleton,
		Animation
	};

	class OzzImporter
	{
	public:
		static bool ImportGltf(const std::string& filePath, const std::string& exePath = "");
	};
}