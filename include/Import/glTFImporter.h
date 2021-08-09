#pragma once
#include "Common/Common.h"
#include "Utils/GLM.h"

#include <vector>
#include <string>
#include <limits>

namespace tinygltf
{
	class Model;
}

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	struct glTFNode
	{
		Ref<glTFNode>                    Parent;
		uint32_t                         Index;
		std::vector<Ref<glTFNode>>       Children;
		glm::vec3                        Translation{};
		glm::vec3                        Scale{ 1.0f };
		glm::quat                        Rotation{};
		int32_t                          Skin = -1;
		glm::mat4                        Matrix;

		glm::mat4                        GetLocalMatrix();
	};

	struct Skin
	{
		std::string                      Name;
		Ref<glTFNode>                    SkeletonRoot = nullptr;
		std::vector<glm::mat4>           InverseBindMatrices;
		std::vector<Ref<glTFNode>>       Joints;
	};								     

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

	struct ImportedAnimationGlTF
	{
		std::vector<Skin>                Skins;
		std::vector<Ref<glTFNode>>       Nodes;
		std::vector<glm::mat4>           Joints;
		glm::mat4                        GetNodeMatrix(Ref<glTFNode>& node);
		void                             UpdateJoints(Ref<glTFNode>& node);
		Ref<glTFNode>                    NodeFromIndex(uint32_t index, ImportedAnimationGlTF* data);
	};

	class glTFImporter
	{
	public:

		static bool Import(const std::string& filePath, ImportedDataGlTF* out_data);
		static bool ImportAnimation(const std::string& filePath, ImportedAnimationGlTF* out_data);
		static bool ImportFromString(const std::string& src, ImportedDataGlTF* out_data);

	private:
		
		static void Import(tinygltf::Model* model, ImportedDataGlTF* out_data);
	};
}