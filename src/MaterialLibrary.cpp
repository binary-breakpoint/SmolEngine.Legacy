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
				JobsSystemInstance::Schedule([&]() {newMaterial.AlbedroTexIndex = AddTexture(infoCI->Textures[MaterialTexture::Albedro], newMaterial.UseAlbedroTex); });
				JobsSystemInstance::Schedule([&]() {newMaterial.NormalTexIndex = AddTexture(infoCI->Textures[MaterialTexture::Normal], newMaterial.UseNormalTex); });
				JobsSystemInstance::Schedule([&]() {newMaterial.MetallicTexIndex = AddTexture(infoCI->Textures[MaterialTexture::Metallic], newMaterial.UseMetallicTex); });
				JobsSystemInstance::Schedule([&]() {newMaterial.RoughnessTexIndex = AddTexture(infoCI->Textures[MaterialTexture::Roughness], newMaterial.UseRoughnessTex); });
				JobsSystemInstance::Schedule([&]() {newMaterial.AOTexIndex = AddTexture(infoCI->Textures[MaterialTexture::AO], newMaterial.UseAOTex); });
			}
			JobsSystemInstance::EndSubmition();
#else
			newMaterial.AlbedroTexIndex = AddTexture(infoCI->Textures[MaterialTexture::Albedro], newMaterial.UseAlbedroTex);
			newMaterial.NormalTexIndex = AddTexture(infoCI->Textures[MaterialTexture::Normal], newMaterial.UseNormalTex);
			newMaterial.MetallicTexIndex = AddTexture(infoCI->Textures[MaterialTexture::Metallic], newMaterial.UseMetallicTex);
			newMaterial.RoughnessTexIndex = AddTexture(infoCI->Textures[MaterialTexture::Roughness], newMaterial.UseRoughnessTex);
			newMaterial.AOTexIndex = AddTexture(infoCI->Textures[MaterialTexture::AO], newMaterial.UseAOTex);
#endif

			// x - Metallic, y - Roughness, z - Albedro
			newMaterial.PBRValues.x = infoCI->Metallic;
			newMaterial.PBRValues.y = infoCI->Roughness;
			newMaterial.PBRValues.z = infoCI->Albedro;
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

	bool MaterialLibrary::Load(const std::string& filePath, MaterialCreateInfo& out_info, const std::string& searchPath)
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

		if (searchPath.empty() == false)
		{
			for (auto& [type, texPath] : out_info.Textures)
			{
				std::filesystem::path origPath(texPath);
				std::string origName = origPath.filename().stem().u8string();

				for (auto& p : std::filesystem::recursive_directory_iterator(searchPath))
				{
					std::string name = p.path().filename().stem().u8string();
					if (name == origName)
					{
						texPath = p.path().u8string();
						break;
					}
				}
			}

			Save(filePath, out_info);
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

	MaterialCreateInfo::MaterialCreateInfo()
	{
		Textures.reserve(5);
	}

	MaterialCreateInfo::~MaterialCreateInfo()
	{

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
		Textures[type] = filePath.c_str();
	}

	float* MaterialCreateInfo::GetMetalness()
	{
		return &Metallic;
	}

	float* MaterialCreateInfo::GetRoughness()
	{
		return &Roughness;
	}

	std::unordered_map<MaterialTexture, std::string>& MaterialCreateInfo::GetTexturesInfo()
	{
		return Textures;
	}
}