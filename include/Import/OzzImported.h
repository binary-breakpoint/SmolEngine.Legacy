#pragma once

#include <string>
#include <unordered_map>

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
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
		static bool ImportGltf(const std::string& filePath);
	};
}