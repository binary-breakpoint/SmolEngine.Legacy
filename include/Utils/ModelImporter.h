#pragma once
#include "Common/Core.h"
#include "Common/Shared.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Frostium
{
	struct ImportedComponent
	{
		std::string                       Name = "";

		std::vector<PBRVertex>            VertexData;
		std::vector<uint32_t>             Indices;
	};

	struct ImportedData
	{
		std::vector<ImportedComponent>                   Components;
	};

	enum class ModelImporterFlags: uint16_t
	{
		Mesh,
		SkeletalAnimation,
		VertexAnimation
	};

	class ModelImporter
	{
	public:

		static bool Load(const std::string& filePath, ImportedData* out_data,
			ModelImporterFlags flags = ModelImporterFlags::Mesh);
	};
}