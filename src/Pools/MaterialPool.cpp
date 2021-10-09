#include "stdafx.h"
#include "Pools/MaterialPool.h"
#include "Common/DebugLog.h"

#include <filesystem>
#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/archives/json.hpp>

#include "Multithreading/JobsSystemInstance.h"
#include "Renderer/RendererDeferred.h"

namespace SmolEngine
{
	const uint32_t maxTextures = 4096;

	MaterialPool::MaterialPool()
	{
		s_Instance = this;
		m_Textures.resize(maxTextures);

		MaterialCreateInfo materialInfo = {};
		materialInfo.SetMetalness(0.2f);
		materialInfo.SetRoughness(1.0f);
		Add(&materialInfo, "default material");
	}

	MaterialPool::~MaterialPool()
	{
		s_Instance = nullptr;
	}

	uint32_t MaterialPool::Add(MaterialCreateInfo* infoCI, const std::string& name)
	{
		uint32_t materialID = 0;
		size_t hashID = s_Instance->m_Hash(name);
		if (s_Instance->m_MaterialMap.size() > 0)
		{
			auto& it = s_Instance->m_MaterialMap.find(hashID);
			if (it != s_Instance->m_MaterialMap.end())
			{
				return it->second;
			}
		}

		PBRMaterial newMaterial = {};
		newMaterial.AlbedroTexIndex = s_Instance->AddTexture(&infoCI->AlbedroTex, newMaterial.UseAlbedroTex);
		newMaterial.NormalTexIndex = s_Instance->AddTexture(&infoCI->NormalTex, newMaterial.UseNormalTex);
		newMaterial.MetallicTexIndex = s_Instance->AddTexture(&infoCI->MetallnessTex, newMaterial.UseMetallicTex);
		newMaterial.RoughnessTexIndex = s_Instance->AddTexture(&infoCI->RoughnessTex, newMaterial.UseRoughnessTex);
		newMaterial.AOTexIndex = s_Instance->AddTexture(&infoCI->AOTex, newMaterial.UseAOTex);
		newMaterial.EmissiveTexIndex = s_Instance->AddTexture(&infoCI->EmissiveTex, newMaterial.UseEmissiveTex);
		newMaterial.Metalness = infoCI->Metallness;
		newMaterial.Roughness = infoCI->Roughness;
		newMaterial.EmissionStrength = infoCI->EmissionStrength;
		newMaterial.Albedro = glm::vec4(infoCI->AlbedroColor, 1.0f);

		{
			std::lock_guard<std::mutex> lock(s_Instance->m_Mutex);

			materialID = s_Instance->m_MaterialIndex;
			s_Instance->m_Materials.emplace_back(newMaterial);
			s_Instance->m_MaterialMap.insert({ hashID, materialID });
			s_Instance->m_MaterialIndex++;
		}

		return materialID;
	}

	bool MaterialPool::Remove(const std::string& name)
	{
		return false; // temp
	}

	void MaterialPool::Clear()
	{
		s_Instance->m_MaterialIndex = 0;
		s_Instance->m_TextureIndex = 0;

		s_Instance->m_Textures.clear();
		s_Instance->m_Textures.resize(maxTextures);

		s_Instance->m_Materials.clear();
		s_Instance->m_MaterialMap.clear();
	}

	uint32_t MaterialPool::AddTexture(TextureCreateInfo* info, uint32_t& useTetxure)
	{
		uint32_t index = 0;
		if (info->FilePath.empty() == false)
		{
			Ref<Texture> tex = Texture::Create();
			tex->LoadFromFile(info);
			{
				std::lock_guard<std::mutex> lock(m_Mutex);

				index = m_TextureIndex;
				m_Textures[index] = tex;
				m_TextureIndex++;
			}

			useTetxure = true;
			return index;
		}

		useTetxure = false;
		return index;
	}

	PBRMaterial* MaterialPool::GetMaterial(uint32_t ID)
	{
		if (ID > s_Instance->m_MaterialIndex)
			return nullptr;

		return &s_Instance->m_Materials[ID];
	}

	PBRMaterial* MaterialPool::GetMaterial(std::string& path)
	{
		size_t hashID = s_Instance->m_Hash(path);
		const auto& it = s_Instance->m_MaterialMap.find(hashID);
		if (it == s_Instance->m_MaterialMap.end())
			return nullptr;

		return &s_Instance->m_Materials[it->second];
	}

	int32_t MaterialPool::GetMaterialID(std::string& path)
	{
		size_t hashID = s_Instance->m_Hash(path);
		const auto& it = s_Instance->m_MaterialMap.find(hashID);
		if (it == s_Instance->m_MaterialMap.end())
			return 0;

		return it->second;
	}

	int32_t MaterialPool::GetMaterialID(size_t& hashed_path)
	{
		const auto& it = s_Instance->m_MaterialMap.find(hashed_path);
		if (it == s_Instance->m_MaterialMap.end())
			return 0;

		return it->second;
	}

	std::vector<PBRMaterial>& MaterialPool::GetMaterials()
	{
		return s_Instance->m_Materials;
	}
	void MaterialPool::GetMaterialsPtr(void*& data, uint32_t& size)
	{
		if (s_Instance->m_Materials.size() > 0)
		{
			data = s_Instance->m_Materials.data();
			size = static_cast<uint32_t>(sizeof(PBRMaterial) * s_Instance->m_Materials.size());
		}
	}

	const std::vector<Ref<Texture>>& MaterialPool::GetTextures()
	{
		return s_Instance->m_Textures;
	}

	void MaterialCreateInfo::SetMetalness(float value)
	{
		Metallness = value;
	}

	void MaterialCreateInfo::SetRoughness(float value)
	{
		Roughness = value;
	}

	void MaterialCreateInfo::SetEmissionStrength(float value)
	{
		EmissionStrength = value;
	}

	void MaterialCreateInfo::SetAlbedro(const glm::vec3& color)
	{
		AlbedroColor = color;
	}

	void MaterialCreateInfo::SetTexture(MaterialTexture type, const TextureCreateInfo* info)
	{
		switch (type)
		{
		case MaterialTexture::Albedo:
			AlbedroTex = *info;
			break;
		case MaterialTexture::Normal:
			NormalTex = *info;
			break;
		case MaterialTexture::Metallic:
			MetallnessTex = *info;
			break;
		case MaterialTexture::Roughness:
			RoughnessTex = *info;
			break;
		case MaterialTexture::AO:
			AOTex = *info;
			break;
		case MaterialTexture::Emissive:
			EmissiveTex = *info;
			break;
		default: break;
		}
	}

	void MaterialCreateInfo::GetTextures(std::unordered_map<MaterialTexture, TextureCreateInfo*>& out_hashmap)
	{
		if (AlbedroTex.FilePath.empty() == false)
		{
			out_hashmap[MaterialTexture::Albedo] = &AlbedroTex;
		}

		if (NormalTex.FilePath.empty() == false)
		{
			out_hashmap[MaterialTexture::Normal] = &NormalTex;
		}

		if (MetallnessTex.FilePath.empty() == false)
		{
			out_hashmap[MaterialTexture::Metallic] = &MetallnessTex;
		}

		if (RoughnessTex.FilePath.empty() == false)
		{
			out_hashmap[MaterialTexture::Roughness] = &RoughnessTex;
		}

		if (AOTex.FilePath.empty() == false)
		{
			out_hashmap[MaterialTexture::AO] = &AOTex;
		}

		if (EmissiveTex.FilePath.empty() == false)
		{
			out_hashmap[MaterialTexture::Emissive] = &EmissiveTex;
		}
	}

	bool MaterialCreateInfo::Load(const std::string& filePath)
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

	bool MaterialCreateInfo::Save(const std::string& filePath)
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
}