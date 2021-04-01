#pragma once
#include "Common/Core.h"
#include "Common/Shared.h"

#include <unordered_map>
#include <string>

namespace Frostium
{
	struct GraphicsPipelineShaderCreateInfo
	{
		bool                                 Optimize = false;
		bool                                 UseSingleFile = false;

		std::unordered_map<ShaderType,
			std::string>                     FilePaths;
		std::unordered_map<uint32_t,
			size_t>                          StorageBuffersSizes;
		std::string                          SingleFilePath = "";

	};
}
