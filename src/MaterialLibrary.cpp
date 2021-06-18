#include "stdafx.h"
#include "MaterialLibrary.h"
#include "Common/SLog.h"
#include "Extensions/JobsSystemInstance.h"

#include <filesystem>
#include <mutex>
#include <cereal/archives/json.hpp>

#ifdef FROSTIUM_SMOLENGINE_IMPL
std::mutex* s_Mutex = nullptr;
#endif

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	const uint32_t maxTextures = 4096;
	MaterialLibrary* MaterialLibrary::s_Instance = nullptr;

	MaterialLibrary::MaterialLibrary()
	{
		s_Instance = this;
#ifdef FROSTIUM_SMOLENGINE_IMPL
		s_Mutex = new std::mutex();
#endif
		m_Textures.resize(maxTextures);
	}

	MaterialLibrary::~MaterialLibrary()
	{
		s_Instance = nullptr;
#ifdef FROSTIUM_SMOLENGINE_IMPL
		delete s_Mutex;
#endif
	}

	uint32_t MaterialLibrary::Add(MaterialCreateInfo* infoCI, const std::string& name)
	{
		uint32_t materialID = 0;
		size_t hashID = m_Hash(name);
		if (m_MaterialMap.size() > 0)
		{
			auto& it = m_MaterialMap.find(hashID);
			if (it != m_MaterialMap.end())
			{
				return it->second;
			}
		}

		PBRMaterial newMaterial = {};
		{

#ifdef FROSTIUM_SMOLENGINE_IMPL
			JobsSystemInstance::BeginSubmition();
			{
				JobsSystemInstance::Schedule([&]() {newMaterial.AlbedroTexIndex = AddTexture(infoCI->AlbedroPath, newMaterial.UseAlbedroTex); });
				JobsSystemInstance::Schedule([&]() {newMaterial.NormalTexIndex = AddTexture(infoCI->NormalPath, newMaterial.UseNormalTex); });
				JobsSystemInstance::Schedule([&]() {newMaterial.MetallicTexIndex = AddTexture(infoCI->MetallnessPath, newMaterial.UseMetallicTex); });
				JobsSystemInstance::Schedule([&]() {newMaterial.RoughnessTexIndex = AddTexture(infoCI->RoughnessPath, newMaterial.UseRoughnessTex); });
				JobsSystemInstance::Schedule([&]() {newMaterial.AOTexIndex = AddTexture(infoCI->AOPath, newMaterial.UseAOTex); });
				JobsSystemInstance::Schedule([&]() {newMaterial.EmissiveTexIndex = AddTexture(infoCI->EmissivePath, newMaterial.UseEmissiveTex); });
				JobsSystemInstance::Schedule([&]() {newMaterial.HeightTexIndex = AddTexture(infoCI->HeightPath, newMaterial.UseHeightTex); });
			}
			JobsSystemInstance::EndSubmition();
#else
			newMaterial.AlbedroTexIndex = AddTexture(infoCI->AlbedroPath, newMaterial.UseAlbedroTex);
			newMaterial.NormalTexIndex = AddTexture(infoCI->NormalPath, newMaterial.UseNormalTex);
			newMaterial.MetallicTexIndex = AddTexture(infoCI->MetallnessPath, newMaterial.UseMetallicTex);
			newMaterial.RoughnessTexIndex = AddTexture(infoCI->RoughnessPath, newMaterial.UseRoughnessTex);
			newMaterial.AOTexIndex = AddTexture(infoCI->AOPath, newMaterial.UseAOTex);
			newMaterial.EmissiveTexIndex = AddTexture(infoCI->EmissivePath, newMaterial.UseEmissiveTex);
			newMaterial.HeightTexIndex = AddTexture(infoCI->HeightPath, newMaterial.UseHeightTex);
#endif

			newMaterial.Metalness = infoCI->Metallness;
			newMaterial.Roughness = infoCI->Roughness;
			newMaterial.Albedro = glm::vec4(infoCI->AlbedroColor, 1.0f);
		}

		materialID = m_MaterialIndex;
		m_Materials.emplace_back(newMaterial);
		m_MaterialMap.insert({ hashID, materialID });
		m_MaterialIndex++;
		return materialID;
	}

	bool MaterialLibrary::Delete(const std::string& name)
	{
		return false; // temp
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

		MaterialCreateInfo copy = out_info;
		storage << file.rdbuf();
		{
			cereal::JSONInputArchive input{ storage };
			input(copy.Metallness, copy.Roughness, copy.AlbedroPath, copy.NormalPath, copy.MetallnessPath, copy.RoughnessPath,
				copy.AOPath, copy.EmissivePath, copy.HeightPath, copy.AlbedroColor.r, copy.AlbedroColor.g, copy.AlbedroColor.b);
		}

		out_info = copy;
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

#ifdef FROSTIUM_SMOLENGINE_IMPL
			{
				const std::lock_guard<std::mutex> lock(*s_Mutex);

				index = m_TextureIndex;
				m_Textures[index] = tex;
				m_TextureIndex++;
			}
#else

			index = m_TextureIndex;
			m_Textures[index] = tex;
			m_TextureIndex++;
#endif

			useTetxure = true;
			return index;
		}

		useTetxure = false;
		return index;
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
		if (m_Materials.size() > 0)
		{
			data = m_Materials.data();
			size = static_cast<uint32_t>(sizeof(PBRMaterial) * m_Materials.size());
		}
	}

	void MaterialLibrary::GetTextures(std::vector<Texture*>& out_textures) const
	{
		out_textures = m_Textures;
	}

	void MaterialCreateInfo::SetMetalness(float value)
	{
		Metallness = value;
	}

	void MaterialCreateInfo::SetRoughness(float value)
	{
		Roughness = value;
	}

	void MaterialCreateInfo::SetAlbedro(const glm::vec3& color)
	{
		AlbedroColor = color;
	}

	void MaterialCreateInfo::SetTexture(MaterialTexture type, const std::string& filePath)
	{
		switch (type)
		{
		case MaterialTexture::Albedro:
			AlbedroPath = filePath;
			break;
		case MaterialTexture::Normal:
			NormalPath = filePath;
			break;
		case MaterialTexture::Metallic:
			MetallnessPath = filePath;
			break;
		case MaterialTexture::Roughness:
			RoughnessPath = filePath;
			break;
		case MaterialTexture::AO:
			AOPath = filePath;
			break;
		case MaterialTexture::Emissive:
			EmissivePath = filePath;
			break;
		case MaterialTexture::Height:
			HeightPath = filePath;
			break;
		default:
			break;
		}
	}

	void MaterialCreateInfo::GetTextures(std::unordered_map<MaterialTexture, std::string*>& out_hashmap)
	{
		if (AlbedroPath.empty() == false)
		{
			out_hashmap[MaterialTexture::Albedro] = &AlbedroPath;
		}

		if (NormalPath.empty() == false)
		{
			out_hashmap[MaterialTexture::Normal] = &NormalPath;
		}

		if (MetallnessPath.empty() == false)
		{
			out_hashmap[MaterialTexture::Metallic] = &MetallnessPath;
		}

		if (RoughnessPath.empty() == false)
		{
			out_hashmap[MaterialTexture::Roughness] = &RoughnessPath;
		}

		if (AOPath.empty() == false)
		{
			out_hashmap[MaterialTexture::AO] = &AOPath;
		}

		if (EmissivePath.empty() == false)
		{
			out_hashmap[MaterialTexture::Emissive] = &EmissivePath;
		}

		if (HeightPath.empty() == false)
		{
			out_hashmap[MaterialTexture::Height] = &HeightPath;
		}
	}
}