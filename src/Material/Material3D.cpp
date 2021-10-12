#include "stdafx.h"
#include "Material/Material3D.h"
#include "Renderer/RendererDeferred.h"

#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/archives/json.hpp>

namespace SmolEngine
{
	bool Material3D::LoadAsDefault(RendererStorage* storage)
	{
		GraphicsPipelineCreateInfo pipelineCI = {};
		MaterialCreateInfoEx materialCI{};
		ShaderCreateInfo shaderCI = {};

		{
			const auto& path = GraphicsContext::GetSingleton()->GetResourcesPath();
			shaderCI.FilePaths[ShaderType::Vertex] = path + "Shaders/Gbuffer.vert";
			shaderCI.FilePaths[ShaderType::Fragment] = path + "Shaders/Gbuffer.frag";
			// SSBO's
			ShaderBufferInfo bufferInfo = {};

			// Vertex
			bufferInfo.Size = sizeof(InstanceData) * max_objects;
			shaderCI.BufferInfos[RendererStorage::m_ShaderDataBinding] = bufferInfo;

			bufferInfo.Size = sizeof(PBRMaterial) * max_materials;
			shaderCI.BufferInfos[RendererStorage::m_MaterialsBinding] = bufferInfo;

			bufferInfo.Size = sizeof(glm::mat4) * max_anim_joints;
			shaderCI.BufferInfos[RendererStorage::m_AnimBinding] = bufferInfo;
		}

		{
			BufferLayout layout =
			{
				{ DataTypes::Float3, "aPos" },
				{ DataTypes::Float3, "aNormal" },
				{ DataTypes::Float3, "aTangent" },
				{ DataTypes::Float2, "aUV" },
				{ DataTypes::Int4,   "aBoneIDs"},
				{ DataTypes::Float4, "aWeight"}
			};

			VertexInputInfo vertex(sizeof(PBRVertex), layout);

			pipelineCI.VertexInputInfos = { vertex };
			pipelineCI.PipelineName = "DefaultMaterial";
			pipelineCI.ShaderCreateInfo = shaderCI;
		}

		materialCI.Name = pipelineCI.PipelineName;
		materialCI.PipelineCreateInfo = pipelineCI;
		materialCI.pStorage = storage;

		return Build(&materialCI);
	}

	Ref<Material3D> Material3D::Create()
	{
		return std::make_shared<Material3D>();
	}

	Ref<Material3D::MaterialInfo> Material3D::AddMaterial(MaterialCreateInfo* infoCI, const std::string& name)
	{
		Ref<MaterialInfo> material = GetMaterial(name);
		if (material != nullptr)
			return material;

		{
			std::hash<std::string_view> hasher{};

			material = std::make_shared<MaterialInfo>();
			material->ID = static_cast<uint32_t>(hasher(name));

			material->Uniform.Metalness = infoCI->Metallness;
			material->Uniform.Roughness = infoCI->Roughness;
			material->Uniform.EmissionStrength = infoCI->EmissionStrength;

			constexpr auto loadFN = [](TextureCreateInfo& ci, Ref<Texture>& out_tetxure, uint32_t& out_state)
			{
				if (ci.FilePath.empty())
				{
					out_tetxure = nullptr;
					out_state = 0;
					return;
				}

				out_tetxure = Texture::Create();
				out_tetxure->LoadFromFile(&ci);
				out_state = 1;
			};

			loadFN(infoCI->AlbedroTex, material->Albedo, material->Uniform.UseAlbedroTex);
			loadFN(infoCI->NormalTex, material->Normal, material->Uniform.UseNormalTex);
			loadFN(infoCI->RoughnessTex, material->Roughness, material->Uniform.UseRoughnessTex);
			loadFN(infoCI->MetallnessTex, material->Metallness, material->Uniform.UseMetallicTex);
			loadFN(infoCI->EmissiveTex, material->Emissive, material->Uniform.UseEmissiveTex);
			loadFN(infoCI->AOTex, material->AO, material->Uniform.UseAOTex);
		}

		m_Mutex.lock();
		{
			m_Materials.emplace_back(material);
			m_IDs[name] = material;
		}
		m_Mutex.unlock();
		return material;
	}

	bool Material3D::RemoveMaterial(const std::string& name)
	{
		auto& it = m_IDs.find(name);
		if (it != m_IDs.end())
		{
			auto& pos = std::find(m_Materials.begin(), m_Materials.end(), it->second);

			m_Materials.erase(pos);
			m_IDs.erase(name);
			return true;
		}

		return false;
	}

	bool Material3D::IsMaterialExist(const std::string& name)
	{
		return m_IDs.find(name) != m_IDs.end();
	}

	uint32_t Material3D::GetMaterialCount() const
	{
		return static_cast<uint32_t>(m_Materials.size());
	}

	Ref<Material3D::MaterialInfo> Material3D::GetMaterial(const std::string& name)
	{
		auto& it = m_IDs.find(name);
		if (it != m_IDs.end())
			return it->second;

		return nullptr;
	}

	void Material3D::ClearMaterials()
	{
		m_Materials.clear();
		m_IDs.clear();
	}

	void Material3D::CreateInfo::SetTexture(Material3D::TextureType type, const TextureCreateInfo* info)
	{
		switch (type)
		{
		case Material3D::TextureType::Albedo:
			AlbedroTex = *info;
			break;
		case Material3D::TextureType::Normal:
			NormalTex = *info;
			break;
		case Material3D::TextureType::Metallic:
			MetallnessTex = *info;
			break;
		case Material3D::TextureType::Roughness:
			RoughnessTex = *info;
			break;
		case Material3D::TextureType::AO:
			AOTex = *info;
			break;
		case Material3D::TextureType::Emissive:
			EmissiveTex = *info;
			break;
		default: break;
		}
	}

	void Material3D::CreateInfo::GetTextures(std::unordered_map<Material3D::TextureType, TextureCreateInfo*>& out_hashmap)
	{

		if (AlbedroTex.FilePath.empty() == false)
		{
			out_hashmap[Material3D::TextureType::Albedo] = &AlbedroTex;
		}

		if (NormalTex.FilePath.empty() == false)
		{
			out_hashmap[Material3D::TextureType::Normal] = &NormalTex;
		}

		if (MetallnessTex.FilePath.empty() == false)
		{
			out_hashmap[Material3D::TextureType::Metallic] = &MetallnessTex;
		}

		if (RoughnessTex.FilePath.empty() == false)
		{
			out_hashmap[Material3D::TextureType::Roughness] = &RoughnessTex;
		}

		if (AOTex.FilePath.empty() == false)
		{
			out_hashmap[Material3D::TextureType::AO] = &AOTex;
		}

		if (EmissiveTex.FilePath.empty() == false)
		{
			out_hashmap[Material3D::TextureType::Emissive] = &EmissiveTex;
		}
	}

	bool Material3D::CreateInfo::Load(const std::string& filePath)
	{
		std::stringstream storage;
		std::ifstream file(filePath);
		if (!file)
		{
			DebugLog::LogError("Could not open the file: {}", filePath);

			return false;
		}

		storage << file.rdbuf();
		{
			cereal::JSONInputArchive input{ storage };
			input(Metallness, Roughness, EmissionStrength, AlbedroTex, NormalTex, MetallnessTex, RoughnessTex,
				AOTex, EmissiveTex, AlbedroColor.r, AlbedroColor.g, AlbedroColor.b);
		}

		return true;
	}

	bool Material3D::CreateInfo::Save(const std::string& filePath)
	{
		std::stringstream storage;
		{
			cereal::JSONOutputArchive output{ storage };
			serialize(output);
		}

		std::ofstream myfile(filePath);
		if (myfile.is_open())
		{
			myfile << storage.str();
			myfile.close();
			return true;
		}

		return false;
	}

	const Material3D::Uniform& Material3D::MaterialInfo::GetUniform() const
	{
		return Uniform;
	}

	uint32_t Material3D::MaterialInfo::GetID() const
	{
		return ID;
	}
}