#include "stdafx.h"
#include "Import/glTFImporter.h"
#include "Utils/Utils.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <stb_image/stb_image.h>
#include <tinygltf/tiny_gltf.h>
#include <glm/gtc/type_ptr.hpp>
#include <gli.hpp>

using namespace tinygltf;

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	Ref<glTFNode> FindNode(const Ref<glTFNode>& parent, uint32_t index)
	{
		Ref<glTFNode> nodeFound = nullptr;
		if (parent->Index == index)
		{
			return parent;
		}

		for (auto& child : parent->Children)
		{
			nodeFound = FindNode(child, index);
			if (nodeFound)
			{
				break;
			}
		}
		return nodeFound;
	}

	void LoadSkins(tinygltf::Model& input, ImportedAnimationGlTF* data)
	{
		data->Skins.resize(input.skins.size());

		for (size_t i = 0; i < input.skins.size(); i++)
		{
			tinygltf::Skin glTFSkin = input.skins[i];

			data->Skins[i].Name = glTFSkin.name;
			// Find the root node of the skeleton
			data->Skins[i].SkeletonRoot = data->NodeFromIndex(glTFSkin.skeleton, data);

			// Find joint nodes
			for (int jointIndex : glTFSkin.joints)
			{
				auto& node = data->NodeFromIndex(jointIndex, data);
				if (node)
				{
					data->Skins[i].Joints.push_back(node);
				}
			}

			// Get the inverse bind matrices from the buffer associated to this skin
			if (glTFSkin.inverseBindMatrices > -1)
			{
				const tinygltf::Accessor& accessor = input.accessors[glTFSkin.inverseBindMatrices];
				const tinygltf::BufferView& bufferView = input.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = input.buffers[bufferView.buffer];
				data->Skins[i].InverseBindMatrices.resize(accessor.count);
				memcpy(data->Skins[i].InverseBindMatrices.data(), &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(glm::mat4));
			}
		}
	}

	void LoadNode(const tinygltf::Node& inputNode, const tinygltf::Model& input, const Ref<glTFNode>& parent,
		uint32_t nodeIndex, ImportedDataGlTF* out_data = nullptr, ImportedAnimationGlTF* out_anim = nullptr)
	{
		glm::vec3 translation = glm::vec3(0);
		if (inputNode.translation.size() == 3)
		{
			translation = glm::make_vec3(inputNode.translation.data());
		}

		glm::vec3 scale = glm::vec3(1);
		if (inputNode.scale.size() == 3)
		{
			scale = glm::make_vec3(inputNode.scale.data());
		}

		glm::quat rotation = glm::mat4(1);
		if (inputNode.rotation.size() == 4)
		{
			rotation = glm::make_quat(inputNode.rotation.data());
		}

		glm::mat4 matrix = glm::mat4(1);
		if (inputNode.matrix.size() == 16)
		{
			matrix = glm::make_mat4x4(inputNode.matrix.data());
		}

		Ref<glTFNode> node = nullptr;
		if (out_anim)
		{
			node = std::make_shared<glTFNode>();
			node->Parent = parent;
			node->Matrix = glm::mat4(1.0f);
			node->Index = nodeIndex;
			node->Skin = inputNode.skin;

			node->Translation = translation;
			node->Rotation = rotation;
			node->Scale = scale;
			node->Matrix = matrix;

			if (parent)
				parent->Children.push_back(node);
			else
			{
				out_anim->Nodes.push_back(node);
			}
		}

		// Load node's children
		if (inputNode.children.size() > 0)
		{
			for (size_t i = 0; i < inputNode.children.size(); i++)
			{
				LoadNode(input.nodes[inputNode.children[i]], input, node, inputNode.children[i], out_data, out_anim);
			}
		}

		if (out_data)
		{
			// If the node contains mesh data, we load vertices and indices from the buffers
		// In glTF this is done via accessors and buffer views
			if (inputNode.mesh > -1)
			{
				const tinygltf::Mesh mesh = input.meshes[inputNode.mesh];
				// Iterate through all primitives of this node's mesh
				for (size_t i = 0; i < mesh.primitives.size(); i++)
				{
					const tinygltf::Primitive& glTFPrimitive = mesh.primitives[i];
					Primitive                  primitive{};
					uint32_t                   firstIndex = 0;
					uint32_t                   vertexStart = 0;
					uint32_t                   indexCount = 0;
					bool                       hasSkin = false;
					glm::mat4                  model = glm::mat4(1.0f);
					glm::vec3                  rot = glm::eulerAngles(rotation);
					glm::vec3                  posMin{};
					glm::vec3                  posMax{};

					Utils::ComposeTransform(translation, rot, scale, model);
					if (out_data->Primitives.size() > 0)
					{
						firstIndex = static_cast<uint32_t>(out_data->Primitives.back().IndexBuffer.size());
						vertexStart = static_cast<uint32_t>(out_data->Primitives.back().VertexBuffer.size());
					}

					// Vertices
					{

						const float* positionBuffer = nullptr;
						const float* normalsBuffer = nullptr;
						const float* tangentsBuffer = nullptr;
						const float* texCoordsBuffer = nullptr;
						const uint16_t* jointIndicesBuffer = nullptr;
						const float* jointWeightsBuffer = nullptr;
						size_t          vertexCount = 0;

						// Get buffer data for vertex positions
						if (glTFPrimitive.attributes.find("POSITION") != glTFPrimitive.attributes.end())
						{
							const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("POSITION")->second];
							const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
							positionBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
							vertexCount = accessor.count;

							posMin = glm::vec3(accessor.minValues[0], accessor.minValues[1], accessor.minValues[2]);
							posMax = glm::vec3(accessor.maxValues[0], accessor.maxValues[1], accessor.maxValues[2]);
						}
						// Get buffer data for vertex normals
						if (glTFPrimitive.attributes.find("NORMAL") != glTFPrimitive.attributes.end())
						{
							const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("NORMAL")->second];
							const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
							normalsBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
						}
						// Get buffer data for vertex tangents
						if (glTFPrimitive.attributes.find("TANGENT") != glTFPrimitive.attributes.end())
						{
							const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("TANGENT")->second];
							const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
							tangentsBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
						}
						// Get buffer data for vertex texture coordinates
						// glTF supports multiple sets, we only load the first one
						if (glTFPrimitive.attributes.find("TEXCOORD_0") != glTFPrimitive.attributes.end())
						{
							const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("TEXCOORD_0")->second];
							const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
							texCoordsBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
						}

						// POI: Get buffer data required for vertex skinning
						// Get vertex joint indices
						if (glTFPrimitive.attributes.find("JOINTS_0") != glTFPrimitive.attributes.end())
						{
							const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("JOINTS_0")->second];
							const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
							jointIndicesBuffer = reinterpret_cast<const uint16_t*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
						}
						// Get vertex joint weights
						if (glTFPrimitive.attributes.find("WEIGHTS_0") != glTFPrimitive.attributes.end())
						{
							const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("WEIGHTS_0")->second];
							const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
							jointWeightsBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
						}

						hasSkin = (jointIndicesBuffer && jointWeightsBuffer);

						// Append data to model's vertex buffer
						for (size_t v = 0; v < vertexCount; v++)
						{
							PBRVertex vert = {};
							vert.Pos = model * glm::vec4(glm::make_vec3(&positionBuffer[v * 3]), 1.0f);
							vert.Normals = glm::normalize(glm::vec3(normalsBuffer ? glm::make_vec3(&normalsBuffer[v * 3]) : glm::vec3(0.0f)));
							vert.Tangent = glm::vec4(tangentsBuffer ? glm::make_vec3(&tangentsBuffer[v * 3]) : glm::vec3(1.0f), 1.0f);
							vert.UVs = texCoordsBuffer ? glm::make_vec2(&texCoordsBuffer[v * 2]) : glm::vec3(0.0f);
							vert.jointIndices = hasSkin ? glm::vec4(glm::make_vec4(&jointIndicesBuffer[v * 4])) : glm::vec4(0.0f);
							vert.jointWeight = hasSkin ? glm::make_vec4(&jointWeightsBuffer[v * 4]) : glm::vec4(0.0f);

							primitive.VertexBuffer.emplace_back(vert);
						}
					}
					// Indices
					{
						const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.indices];
						const tinygltf::BufferView& bufferView = input.bufferViews[accessor.bufferView];
						const tinygltf::Buffer& buffer = input.buffers[bufferView.buffer];

						indexCount += static_cast<uint32_t>(accessor.count);

						// glTF supports different component types of indices
						switch (accessor.componentType)
						{
						case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
							uint32_t* buf = new uint32_t[accessor.count];
							memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint32_t));
							for (size_t index = 0; index < accessor.count; index++)
							{
								primitive.IndexBuffer.push_back(buf[index]);
							}
							break;
						}
						case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
							uint16_t* buf = new uint16_t[accessor.count];
							memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint16_t));
							for (size_t index = 0; index < accessor.count; index++)
							{
								primitive.IndexBuffer.push_back(buf[index]);
							}
							break;
						}
						case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
							uint8_t* buf = new uint8_t[accessor.count];
							memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint8_t));
							for (size_t index = 0; index < accessor.count; index++)
							{
								primitive.IndexBuffer.push_back(buf[index]);
							}
							break;
						}
						default:
							std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
							return;
						}
					}

					primitive.MeshName = inputNode.name;
					primitive.AABB.SetBoundingBox(posMin, posMax);
					primitive.AABB.CalculateAABB(model);
					out_data->Primitives.push_back(primitive);
				}
			}
		}
	}

	glm::mat4 glTFNode::GetLocalMatrix()
	{
		return glm::translate(glm::mat4(1.0f), Translation) * glm::mat4(Rotation) * glm::scale(glm::mat4(1.0f), Scale) * Matrix;
	}

	bool glTFImporter::Import(const std::string& filePath, ImportedDataGlTF* out_data)
	{
		tinygltf::Model    glTFInput;
		tinygltf::TinyGLTF gltfContext;
		std::string        error, warning;

		bool fileLoaded = gltfContext.LoadASCIIFromFile(&glTFInput, &error, &warning, filePath);
		if (fileLoaded) { Import(&glTFInput, out_data); }
		return fileLoaded;
	}

	bool glTFImporter::ImportAnimation(const std::string& filePath, ImportedAnimationGlTF* out_data)
	{
		tinygltf::Model    glTFInput;
		tinygltf::TinyGLTF gltfContext;
		std::string        error, warning;

		bool fileLoaded = gltfContext.LoadASCIIFromFile(&glTFInput, &error, &warning, filePath);
		if (fileLoaded)
		{
			const tinygltf::Scene& scene = glTFInput.scenes[0];
			for (size_t i = 0; i < scene.nodes.size(); i++)
			{
				const tinygltf::Node node = glTFInput.nodes[scene.nodes[i]];
				LoadNode(node, glTFInput, nullptr, scene.nodes[i], nullptr, out_data);
			}
		}

		LoadSkins(glTFInput, out_data);
		for (auto& node : out_data->Nodes)
			out_data->UpdateJoints(node);

		return fileLoaded;
	}

	bool glTFImporter::ImportFromString(const std::string& src, ImportedDataGlTF* out_data)
	{
		tinygltf::Model    glTFInput;
		tinygltf::TinyGLTF gltfContext;
		std::string        error, warning;

		bool fileLoaded = gltfContext.LoadASCIIFromString(&glTFInput, &error, &warning, src.c_str(), static_cast<unsigned int>(strlen(src.c_str())), GetBaseDir(src));
		if (fileLoaded) { Import(&glTFInput, out_data); }
		return fileLoaded;
	}

	void glTFImporter::Import(tinygltf::Model* model, ImportedDataGlTF* out_data)
	{
		const tinygltf::Scene& scene = model->scenes[0];
		for (size_t i = 0; i < scene.nodes.size(); i++)
		{
			const tinygltf::Node node = model->nodes[scene.nodes[i]];
			LoadNode(node, *model, nullptr, scene.nodes[i], out_data);
		}
	}

	glm::mat4 ImportedAnimationGlTF::GetNodeMatrix(Ref<glTFNode>& node)
	{
		glm::mat4 matrix = node->GetLocalMatrix();
		auto& currentParent = node->Parent;
		while (currentParent)
		{
			matrix = currentParent->GetLocalMatrix() * matrix;
			currentParent = currentParent->Parent;
		}

		return matrix;
	}

	void ImportedAnimationGlTF::UpdateJoints(Ref<glTFNode>& node)
	{
		auto& kek = *this;

		if (node->Skin > -1)
		{
			// Update the joint matrices
			glm::mat4 inverseTransform = glm::inverse(GetNodeMatrix(node));
			Skin&     skin = Skins[node->Skin];
			size_t    numJoints = (uint32_t)skin.Joints.size();

			Joints.resize(numJoints);
			for (size_t i = 0; i < numJoints; i++)
			{
				Joints[i] *= skin.InverseBindMatrices[i];
			}
		}

		for (auto& child : node->Children)
		{
			UpdateJoints(child);
		}
	}

	Ref<glTFNode> ImportedAnimationGlTF::NodeFromIndex(uint32_t index, ImportedAnimationGlTF* data)
	{
		Ref<glTFNode> nodeFound = nullptr;
		for (auto& node : data->Nodes)
		{
			nodeFound = FindNode(node, index);
			if (nodeFound)
			{
				break;
			}
		}

		return nodeFound;
	}
}