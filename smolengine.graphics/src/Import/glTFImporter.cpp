#include "stdafx.h"
#include "Import/glTFImporter.h"
#include "Tools/Utils.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <stb_image/stb_image.h>
#include <tinygltf/tiny_gltf.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <gli.hpp>

using namespace tinygltf;

namespace SmolEngine
{
	void LoadNode(const tinygltf::Node& inputNode, const tinygltf::Model& input, uint32_t nodeIndex, ImportedDataGlTF* out_data)
	{
		// Load node's children
		if (inputNode.children.size() > 0)
		{
			for (size_t i = 0; i < inputNode.children.size(); i++)
			{
				LoadNode(input.nodes[inputNode.children[i]], input, inputNode.children[i], out_data);
			}
		}

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

		glm::quat rotation = glm::mat4(0);
		if (inputNode.rotation.size() == 4)
		{
			rotation = glm::make_quat(inputNode.rotation.data());
		}

		glm::mat4 model = glm::mat4(1.0f);
		glm::vec3 rot = glm::vec3(0);

		Utils::ComposeTransform(translation, rot, scale, model);
		model = model * glm::toMat4(rotation);

		// In glTF this is done via accessors and buffer views
		// If the node contains mesh data, we load vertices and indices from the buffers
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
				glm::vec3                  posMin{};
				glm::vec3                  posMax{};

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
				primitive.AABB.MinPoint(posMin);
				primitive.AABB.MaxPoint(posMax);
				primitive.AABB.Transform(model);

				out_data->Primitives.push_back(std::move(primitive));
			}
		}
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

	bool glTFImporter::ImportInverseBindMatrices(const std::string& filePath, std::vector<glm::mat4>& matrices)
	{
		tinygltf::Model    glTFInput;
		tinygltf::TinyGLTF gltfContext;
		std::string        error, warning;

		bool fileLoaded = gltfContext.LoadASCIIFromFile(&glTFInput, &error, &warning, filePath);
		if (fileLoaded)
		{
			for (size_t i = 0; i < glTFInput.skins.size(); i++)
			{
				tinygltf::Skin glTFSkin = glTFInput.skins[i];

				// Get the inverse bind matrices from the buffer associated to this skin
				if (glTFSkin.inverseBindMatrices > -1)
				{
					const tinygltf::Accessor& accessor = glTFInput.accessors[glTFSkin.inverseBindMatrices];
					const tinygltf::BufferView& bufferView = glTFInput.bufferViews[accessor.bufferView];
					const tinygltf::Buffer& buffer = glTFInput.buffers[bufferView.buffer];

					matrices.resize(accessor.count);
					memcpy(matrices.data(), &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(glm::mat4));
				}

				return true;
			}
		}

		return false;
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
			LoadNode(node, *model, scene.nodes[i], out_data);
		}
	}
}