#pragma once
#include "Common/Common.h"
#include "Tools/GLM.h"

#include <vector>
#include <string>
#include <limits>

namespace tinygltf
{
	class Model;
}

namespace SmolEngine
{					    
	struct Primitive
	{
		std::string                      MeshName = "";
		std::vector<PBRVertex>           VertexBuffer;
		std::vector<uint32_t>            IndexBuffer;
		BoundingBox                      AABB;
	};

	struct ImportedDataGlTF
	{
		std::vector<Primitive>           Primitives;
	};

	class glTFImporter
	{
	public:

		static bool Import(const std::string& filePath, ImportedDataGlTF* out_data);
		static bool ImportInverseBindMatrices(const std::string& filePath, std::vector<glm::mat4>& matrices);
		static bool ImportFromString(const std::string& src, ImportedDataGlTF* out_data);

	private:
		
		static void Import(tinygltf::Model* model, ImportedDataGlTF* out_data);
	};
}