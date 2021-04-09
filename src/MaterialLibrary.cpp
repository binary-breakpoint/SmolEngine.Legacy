#include "stdafx.h"
#include "MaterialLibrary.h"
#include "Common/SLog.h"

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

	int32_t MaterialLibrary::Add(MaterialCreateInfo* infoCI, const std::string& path)
	{
		int32_t materialID = -1;
		if (path.empty())
		{
			NATIVE_ERROR("Material path is empty");
			return materialID;
		}

		size_t hashID = m_Hash(path);
		const auto& name_it = m_MaterialMap.find(hashID);
		if (name_it != m_MaterialMap.end())
			return name_it->second;

		Material newMaterial = {};
		{
			newMaterial.m_MaterialProperties.PBRValues.x = infoCI->Metallic;
			newMaterial.m_MaterialProperties.PBRValues.y = infoCI->Roughness;
			newMaterial.m_MaterialProperties.PBRValues.z = infoCI->Albedro;

			newMaterial.m_MaterialProperties.Indexes_1.x = AddTexture(infoCI->Textures[MaterialTexture::Albedro]);
			newMaterial.m_MaterialProperties.Indexes_1.y = AddTexture(infoCI->Textures[MaterialTexture::Normal]);
			newMaterial.m_MaterialProperties.Indexes_1.z = AddTexture(infoCI->Textures[MaterialTexture::Metallic]);
			newMaterial.m_MaterialProperties.Indexes_1.w = AddTexture(infoCI->Textures[MaterialTexture::Roughness]);
			newMaterial.m_MaterialProperties.Indexes_2.x = AddTexture(infoCI->Textures[MaterialTexture::AO]);

			newMaterial.m_MaterialProperties.States_1.x = newMaterial.m_MaterialProperties.Indexes_1.x > -1 ? true : false;
			newMaterial.m_MaterialProperties.States_1.y = newMaterial.m_MaterialProperties.Indexes_1.y > -1 ? true : false;
			newMaterial.m_MaterialProperties.States_1.z = newMaterial.m_MaterialProperties.Indexes_1.z > -1 ? true : false;
			newMaterial.m_MaterialProperties.States_1.w = newMaterial.m_MaterialProperties.Indexes_1.w > -1 ? true : false;
			newMaterial.m_MaterialProperties.States_2.x = newMaterial.m_MaterialProperties.Indexes_2.x > -1 ? true : false;
		}

		materialID = m_MaterialIndex;
		m_Materials.emplace_back(newMaterial);
		m_MaterialMap[hashID] = materialID;
		m_MaterialIndex++;
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

		m_Textures.clear();
		m_Textures.resize(maxTextures);

		m_Materials.clear();
		m_MaterialMap.clear();
	}

	bool MaterialLibrary::Load(std::string& filePath, MaterialCreateInfo& out_info)
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
				out_info.Roughness, out_info.TexturesFilePaths);
		}

		return true;
	}

	bool MaterialLibrary::Save(std::string& filePath, MaterialCreateInfo& info)
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

	int32_t MaterialLibrary::AddTexture(Texture* texture)
	{
		int32_t index = -1;
		if (texture)
		{
			index = m_TextureIndex;
			m_Textures[index] = texture;

			m_TextureIndex++;
			return index;
		}

		return index;
	}

	Material* MaterialLibrary::GetMaterial(int32_t ID)
	{
		if (ID > m_MaterialIndex)
			return nullptr;

		return &m_Materials[ID];
	}

	Material* MaterialLibrary::GetMaterial(std::string& path)
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

	std::vector<Material>& MaterialLibrary::GetMaterials()
	{
		return m_Materials;
	}
	void MaterialLibrary::GetMaterialsPtr(void*& data, uint32_t& size)
	{
		data = m_Materials.data();
		size = static_cast<uint32_t>(sizeof(Material) * m_Materials.size());
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

	void MaterialCreateInfo::SetTexture(MaterialTexture type, Texture* texture, const std::string& filePath)
	{
		Textures[type] = texture;
		TexturesFilePaths[type] = filePath;
	}
}