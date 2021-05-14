#include "stdafx.h"
#include "Utils/glTFImporter.h"
#include "Utils/Utils.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <stb_image/stb_image.h>
#include <tinygltf/tiny_gltf.h>
#include <glm/gtc/type_ptr.hpp>
#include <gli.hpp>

using namespace tinygltf;

namespace Frostium
{
	glTFNode* FindNode(glTFNode* parent, uint32_t index)
	{
		glTFNode* nodeFound = nullptr;
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

	glTFNode* NodeFromIndex(uint32_t index, ImportedDataGlTF* data)
	{
		glTFNode* nodeFound = nullptr;
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

	void LoadSkins(tinygltf::Model& input, ImportedDataGlTF* data)
	{
		data->Skins.resize(input.skins.size());

		for (size_t i = 0; i < input.skins.size(); i++)
		{
			tinygltf::Skin glTFSkin = input.skins[i];

			data->Skins[i].Name = glTFSkin.name;
			// Find the root node of the skeleton
			data->Skins[i].SkeletonRoot = NodeFromIndex(glTFSkin.skeleton, data);

			// Find joint nodes
			for (int jointIndex : glTFSkin.joints)
			{
				glTFNode* node = NodeFromIndex(jointIndex, data);
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

	void LoadAnimations(tinygltf::Model& input, ImportedDataGlTF* data)
	{
		auto& animations = data->Animations;
		animations.resize(input.animations.size());

		for (size_t i = 0; i < input.animations.size(); i++)
		{
			tinygltf::Animation glTFAnimation = input.animations[i];
			animations[i].Name = glTFAnimation.name;

			// Samplers
			animations[i].Samplers.resize(glTFAnimation.samplers.size());
			for (size_t j = 0; j < glTFAnimation.samplers.size(); j++)
			{
				tinygltf::AnimationSampler glTFSampler = glTFAnimation.samplers[j];
				AnimationSampler& dstSampler = animations[i].Samplers[j];
				dstSampler.Interpolation = glTFSampler.interpolation;

				// Read sampler keyframe input time values
				{
					const tinygltf::Accessor& accessor = input.accessors[glTFSampler.input];
					const tinygltf::BufferView& bufferView = input.bufferViews[accessor.bufferView];
					const tinygltf::Buffer& buffer = input.buffers[bufferView.buffer];
					const void* dataPtr = &buffer.data[accessor.byteOffset + bufferView.byteOffset];
					const float* buf = static_cast<const float*>(dataPtr);
					for (size_t index = 0; index < accessor.count; index++)
					{
						dstSampler.Inputs.push_back(buf[index]);
					}
					// Adjust animation's start and end times
					for (auto input : animations[i].Samplers[j].Inputs)
					{
						if (input < animations[i].Properties.GetStart())
						{
							animations[i].Properties.SetStart(input);
						};
						if (input > animations[i].Properties.GetEnd())
						{
							animations[i].Properties.SetEnd(input);
						}
					}
				}

				// Read sampler keyframe output translate/rotate/scale values
				{
					const tinygltf::Accessor& accessor = input.accessors[glTFSampler.output];
					const tinygltf::BufferView& bufferView = input.bufferViews[accessor.bufferView];
					const tinygltf::Buffer& buffer = input.buffers[bufferView.buffer];
					const void* dataPtr = &buffer.data[accessor.byteOffset + bufferView.byteOffset];
					switch (accessor.type)
					{
					case TINYGLTF_TYPE_VEC3: {
						const glm::vec3* buf = static_cast<const glm::vec3*>(dataPtr);
						for (size_t index = 0; index < accessor.count; index++)
						{
							dstSampler.OutputsVec4.push_back(glm::vec4(buf[index], 0.0f));
						}
						break;
					}
					case TINYGLTF_TYPE_VEC4: {
						const glm::vec4* buf = static_cast<const glm::vec4*>(dataPtr);
						for (size_t index = 0; index < accessor.count; index++)
						{
							dstSampler.OutputsVec4.push_back(buf[index]);
						}
						break;
					}
					default: {
						std::cout << "unknown type" << std::endl;
						break;
					}
					}
				}

			}

			// Channels
			animations[i].Channels.resize(glTFAnimation.channels.size());
			for (size_t j = 0; j < glTFAnimation.channels.size(); j++)
			{
				tinygltf::AnimationChannel glTFChannel = glTFAnimation.channels[j];
				AnimationChannel& dstChannel = animations[i].Channels[j];
				dstChannel.Path = glTFChannel.target_path;
				dstChannel.SamplerIndex = glTFChannel.sampler;
				dstChannel.Node = NodeFromIndex(glTFChannel.target_node, data);
			}
		}

	}

	void LoadNode(const tinygltf::Node& inputNode, const tinygltf::Model& input, glTFNode* parent,
		uint32_t nodeIndex, ImportedDataGlTF* out_data)
	{
		glTFNode* node = new glTFNode();
		node->Parent = parent;
		node->Matrix = glm::mat4(1.0f);
		node->Index = nodeIndex;
		node->Skin = inputNode.skin;

		// Get the local node matrix
		// It's either made up from translation, rotation, scale or a 4x4 matrix
		if (inputNode.translation.size() == 3)
		{
			node->Translation = glm::make_vec3(inputNode.translation.data());
		}
		if (inputNode.rotation.size() == 4)
		{
			glm::quat q = glm::make_quat(inputNode.rotation.data());
			node->Rotation = glm::mat4(q);
		}
		if (inputNode.scale.size() == 3)
		{
			node->Scale = glm::make_vec3(inputNode.scale.data());
		}
		if (inputNode.matrix.size() == 16)
		{
			node->Matrix = glm::make_mat4x4(inputNode.matrix.data());
		};

		// Load node's children
		if (inputNode.children.size() > 0)
		{
			for (size_t i = 0; i < inputNode.children.size(); i++)
			{
				LoadNode(input.nodes[inputNode.children[i]], input, node, inputNode.children[i], out_data);
			}
		}

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
				glm::vec3                  rot = glm::eulerAngles(node->Rotation);

				Utils::ComposeTransform(node->Translation, rot, node->Scale, model);
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

				out_data->Primitives.push_back(primitive);
				BoundingBox& box = out_data->Primitives.back().AABB;
				box.CalculateAABB(model);
			}
		}

		if (parent)
			parent->Children.push_back(node);
		else
		{
			out_data->Nodes.push_back(node);
		}
	}

	glm::mat4 glTFNode::GetLocalMatrix()
	{
		return glm::translate(glm::mat4(1.0f), Translation) * glm::mat4(Rotation) * glm::scale(glm::mat4(1.0f), Scale) * Matrix;
	}

	ImportedDataGlTF::~ImportedDataGlTF()
	{
		for (const auto& n : Nodes)
			delete n;
	}

	void ImportedDataGlTF::UpdateJoints(glTFNode* node, AnimationProperties* data)
	{
		if (node->Skin> -1 && data)
		{
			// Update the joint matrices
			glm::mat4              inverseTransform = glm::inverse(GetNodeMatrix(node));
			Skin                   skin = Skins[node->Skin];
			size_t                 numJoints = (uint32_t)skin.Joints.size();


			std::vector<glm::mat4>& joints = data->Joints;
			joints.resize(numJoints);
			for (size_t i = 0; i < numJoints; i++)
			{
				joints[i] = GetNodeMatrix(skin.Joints[i]) * skin.InverseBindMatrices[i];
				joints[i] = inverseTransform * joints[i];
			}
		}

		for (auto& child : node->Children)
		{
			UpdateJoints(child, data);
		}
	}

	void ImportedDataGlTF::UpdateAnimation()
	{
		float deltaTime = GraphicsContext::GetSingleton()->GetDeltaTime();
		if (ActiveAnimation > static_cast<uint32_t>(Animations.size()) - 1)
		{
			NATIVE_INFO("No animation with index {}", ActiveAnimation);
			return;
		}

		glTFAnimation& animation = Animations[ActiveAnimation];
		AnimationProperties* selectedProperties = &animation.Properties;

		selectedProperties->CurrentTime += deltaTime * selectedProperties->Speed;
		if (selectedProperties->CurrentTime > selectedProperties->End)
		{
			selectedProperties->CurrentTime -= selectedProperties->End;
		}

		for (auto& channel : animation.Channels)
		{
			AnimationSampler& sampler = animation.Samplers[channel.SamplerIndex];
			for (size_t i = 0; i < sampler.Inputs.size() - 1; i++)
			{
				if (sampler.Interpolation != "LINEAR")
				{
					NATIVE_INFO("Only supports linear interpolations");
					continue;
				}

				// Get the input keyframe values for the current time stamp
				if ((selectedProperties->CurrentTime >= sampler.Inputs[i]) && (selectedProperties->CurrentTime <= sampler.Inputs[i + 1]))
				{
					float a = (selectedProperties->CurrentTime - sampler.Inputs[i]) / (sampler.Inputs[i + 1] - sampler.Inputs[i]);
					if (channel.Path == "translation")
					{
						channel.Node->Translation = glm::mix(sampler.OutputsVec4[i], sampler.OutputsVec4[i + 1], a);
					}
					if (channel.Path == "rotation")
					{
						glm::quat q1;
						q1.x = sampler.OutputsVec4[i].x;
						q1.y = sampler.OutputsVec4[i].y;
						q1.z = sampler.OutputsVec4[i].z;
						q1.w = sampler.OutputsVec4[i].w;

						glm::quat q2;
						q2.x = sampler.OutputsVec4[i + 1].x;
						q2.y = sampler.OutputsVec4[i + 1].y;
						q2.z = sampler.OutputsVec4[i + 1].z;
						q2.w = sampler.OutputsVec4[i + 1].w;

						channel.Node->Rotation = glm::normalize(glm::slerp(q1, q2, a));
					}
					if (channel.Path == "scale")
					{
						channel.Node->Scale = glm::mix(sampler.OutputsVec4[i], sampler.OutputsVec4[i + 1], a);
					}
				}
			}
		}

		for (auto& node : Nodes)
		{
			UpdateJoints(node, selectedProperties);
		}
	}

	void ImportedDataGlTF::ResetAnimation(uint32_t index)
	{
		glTFAnimation& animation = Animations[index];
		animation.Properties.Active = false;
		animation.Properties.CurrentTime = animation.Properties.Start;
		UpdateAnimation();
	}

	glm::mat4 ImportedDataGlTF::GetNodeMatrix(glTFNode* node)
	{
		glm::mat4              nodeMatrix = node->GetLocalMatrix();
		glTFNode* currentParent = node->Parent;
		while (currentParent)
		{
			nodeMatrix = currentParent->GetLocalMatrix() * nodeMatrix;
			currentParent = currentParent->Parent;
		}

		return nodeMatrix;
	}

	bool glTFImporter::Import(const std::string& filePath, ImportedDataGlTF* out_data)
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
				LoadNode(node, glTFInput, nullptr, scene.nodes[i], out_data);
			}
			LoadSkins(glTFInput, out_data);
			LoadAnimations(glTFInput, out_data);

			glTFAnimation* activeAnim = nullptr;
			if (out_data->Animations.size() > 0)
				activeAnim = &out_data->Animations[out_data->ActiveAnimation];

			// Calculate initial pose
			for (auto node : out_data->Nodes)
			{
				out_data->UpdateJoints(node, &activeAnim->Properties);
			}
		}

		return fileLoaded;
	}
}