#include "stdafx.h"
#include "MaterialLibrary.h"
#include "Common/SLog.h"

#include <filesystem>
#include <cereal/archives/json.hpp>

namespace Frostium
{
	const uint32_t maxTextures = 4096;
	MaterialLibrary* MaterialLibrary::s_Instance = nullptr;

	MaterialLibrary::MaterialLibrary()
	{
		s_Instance = this;
		m_Textures.resize(maxTextures);
	}

	MaterialLibrary::~MaterialLibrary()
	{
		s_Instance = nullptr;
	}

	uint32_t MaterialLibrary::Add(MaterialCreateInfo* infoCI, const std::string& path)
	{
		uint32_t materialID = 0;

		std::string name = GetCompleteName(infoCI);
		size_t hashID = m_Hash(name);
		const auto& name_it = m_MaterialMap.find(hashID);
		if (name_it != m_MaterialMap.end())
			return name_it->second;

		PBRMaterial newMaterial = {};
		{
			newMaterial.AlbedroTexIndex = AddTexture(infoCI->Textures[MaterialTexture::Albedro], newMaterial.UseAlbedroTex);
			newMaterial.NormalTexIndex = AddTexture(infoCI->Textures[MaterialTexture::Normal], newMaterial.UseNormalTex);
			newMaterial.MetallicTexIndex = AddTexture(infoCI->Textures[MaterialTexture::Metallic], newMaterial.UseMetallicTex);
			newMaterial.RoughnessTexIndex = AddTexture(infoCI->Textures[MaterialTexture::Roughness], newMaterial.UseRoughnessTex);
			newMaterial.AOTexIndex = AddTexture(infoCI->Textures[MaterialTexture::AO], newMaterial.UseAOTex);

			// x - Metallic, y - Roughness, z - Albedro
			newMaterial.PBRValues.x = infoCI->Metallic;
			newMaterial.PBRValues.y = infoCI->Roughness;
			newMaterial.PBRValues.z = infoCI->Albedro;
		}

		materialID = m_MaterialIndex;
		m_Materials.emplace_back(newMaterial);
		m_MaterialMap[hashID] = materialID;
		m_MaterialIndex++;

		if(!path.empty())
			Save(path, *infoCI);

		return materialID;
	}

	bool MaterialLibrary::Delete(const std::string& name)
	{
		return false;
	}

	void MaterialLibrary::Reset()
	{
		m_MaterialIndex = 0;
		m_TextureIndex = 0;

		for (const auto& tex : m_Textures)
		{
			if(tex)
				delete tex;
		}

		m_Textures.clear();
		m_Textures.resize(maxTextures);

		m_Materials.clear();
		m_MaterialMap.clear();
	}

	bool MaterialLibrary::Load(const std::string& filePath, MaterialCreateInfo& out_info)
	{
		std::stringstream storage;
		std::ifstream file(filePath);
		if (!file)
		{
			NATIVE_ERROR("Could not open the file: {}", filePath);
			return false;
		}

		storage << file.rdbuf();
		{
			cereal::JSONInputArchive input{ storage };
			input(out_info.Metallic, out_info.Albedro,
				out_info.Roughness, out_info.Textures);
		}

		return true;
	}

	bool MaterialLibrary::Save(const std::string& filePath, MaterialCreateInfo& info)
	{
		std::stringstream storage;
		{
			cereal::JSONOutputArchive output{ storage };
			info.serialize(output);
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

	MaterialLibrary* MaterialLibrary::GetSinglenton()
	{
		return s_Instance;
	}

	uint32_t MaterialLibrary::AddTexture(const std::string& path, uint32_t& useTetxure)
	{
		uint32_t index = 0;
		if (!path.empty())
		{
			Texture* tex = new Texture();
			Texture::Create(path, tex);

			index = m_TextureIndex;
			m_Textures[index] = tex;
			m_TextureIndex++;
			useTetxure = true;
			return index;
		}

		useTetxure = false;
		return index;
	}

	std::string MaterialLibrary::GetCompleteName(MaterialCreateInfo* infoCI)
	{
		std::string result;
		std::filesystem::path p;

		for (auto& [type, path] : infoCI->Textures)
		{
			std::filesystem::path p(path);
			result += p.filename().stem().string();
		}

		return result;
	}

	PBRMaterial* MaterialLibrary::GetMaterial(uint32_t ID)
	{
		if (ID > m_MaterialIndex)
			return nullptr;

		return &m_Materials[ID];
	}

	PBRMaterial* MaterialLibrary::GetMaterial(std::string& path)
	{
		size_t hashID = m_Hash(path);
		const auto& it = m_MaterialMap.find(hashID);
		if (it == m_MaterialMap.end())
			return nullptr;

		return &m_Materials[it->second];
	}

	int32_t MaterialLibrary::GetMaterialID(std::string& path)
	{
		size_t hashID = m_Hash(path);
		const auto& it = m_MaterialMap.find(hashID);
		if (it == m_MaterialMap.end())
			return 0;

		return it->second;
	}

	int32_t MaterialLibrary::GetMaterialID(size_t& hashed_path)
	{
		const auto& it = m_MaterialMap.find(hashed_path);
		if (it == m_MaterialMap.end())
			return 0;

		return it->second;
	}

	std::vector<PBRMaterial>& MaterialLibrary::GetMaterials()
	{
		return m_Materials;
	}
	void MaterialLibrary::GetMaterialsPtr(void*& data, uint32_t& size)
	{
		data = m_Materials.data();
		size = static_cast<uint32_t>(sizeof(PBRMaterial) * m_Materials.size());
	}

	void MaterialLibrary::GetTextures(std::vector<Texture*>& out_textures) const
	{
		out_textures = m_Textures;
	}

	void MaterialCreateInfo::SetMetalness(float value)
	{
		Metallic = value;
	}

	void MaterialCreateInfo::SetRoughness(float value)
	{
		Roughness = value;
	}

	void MaterialCreateInfo::SetAlbedro(float value)
	{
		Albedro = value;
	}

	void MaterialCreateInfo::SetTexture(MaterialTexture type, const std::string& filePath)
	{
		Textures[type] = filePath;
	}
}