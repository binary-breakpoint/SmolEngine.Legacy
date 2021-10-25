#include "stdafx.h"
#include "Renderer/RendererDeferred.h"
#include "Materials/PBRFactory.h"
#include "Materials/MaterialPBR.h"

#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/archives/json.hpp>

namespace SmolEngine
{
	PBRFactory::PBRFactory()
	{
		s_Instance = this;
	}

	PBRFactory::~PBRFactory()
	{
		s_Instance = nullptr;
	}

	Ref<PBRHandle> PBRFactory::AddMaterial(PBRCreateInfo* infoCI, const std::string& name)
	{
		Ref<PBRHandle> material = GetMaterial(name);
		if (material != nullptr)
			return material;

		{
			std::hash<std::string_view> hasher{};

			material = std::make_shared<PBRHandle>();
			material->m_Uniform.ID = static_cast<uint32_t>(hasher(name));

			material->m_Uniform.Metalness = infoCI->Metallness;
			material->m_Uniform.Roughness = infoCI->Roughness;
			material->m_Uniform.EmissionStrength = infoCI->EmissionStrength;
			material->m_Uniform.Albedro = glm::vec4(infoCI->Albedo, 1);

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

			loadFN(infoCI->AlbedroTex, material->m_Albedo, material->m_Uniform.UseAlbedroTex);
			loadFN(infoCI->NormalTex, material->m_Normal, material->m_Uniform.UseNormalTex);
			loadFN(infoCI->RoughnessTex, material->m_Roughness, material->m_Uniform.UseRoughnessTex);
			loadFN(infoCI->MetallnessTex, material->m_Metallness, material->m_Uniform.UseMetallicTex);
			loadFN(infoCI->EmissiveTex, material->m_Emissive, material->m_Uniform.UseEmissiveTex);
			loadFN(infoCI->AOTex, material->m_AO, material->m_Uniform.UseAOTex);
		}

		s_Instance->m_Mutex.lock();
		{
			s_Instance->m_Materials.emplace_back(material);
			s_Instance->m_IDs[name] = material;
		}
		s_Instance->m_Mutex.unlock();

		return material;
	}

	void PBRFactory::ClearMaterials()
	{
		s_Instance->m_Materials.clear();
		s_Instance->m_IDs.clear();

		std::vector<Ref<Texture>> textures(s_Instance->m_MaxTextures);
		RendererStorage::GetDefaultMaterial()->UpdateSamplers(textures, RendererStorage::GetSingleton()->m_TexturesBinding);
	}

	void PBRFactory::UpdateMaterials()
	{
		std::vector<PBRUniform> uniforms(s_Instance->m_Materials.size());
		std::vector<Ref<Texture>> textures(s_Instance->m_MaxTextures);

		constexpr auto addFn = [](const Ref<Texture>& texture, uint32_t& tex_index, uint32_t& global_index, std::vector<Ref<Texture>>& out_textures)
		{
			if (texture != nullptr)
			{
				tex_index = global_index;
				out_textures[global_index] = texture;
				global_index++;
			}
		};

		{
			uint32_t index = 0;

			for (uint32_t i = 0; i < static_cast<uint32_t>(s_Instance->m_Materials.size()); ++i)
			{
				const Ref<PBRHandle>& material = s_Instance->m_Materials[i];

				addFn(material->m_Albedo, material->m_Uniform.AlbedroTexIndex, index, textures);
				addFn(material->m_Normal, material->m_Uniform.NormalTexIndex, index, textures);
				addFn(material->m_Roughness, material->m_Uniform.RoughnessTexIndex, index, textures);
				addFn(material->m_Metallness, material->m_Uniform.MetallicTexIndex, index, textures);
				addFn(material->m_Emissive, material->m_Uniform.EmissiveTexIndex, index, textures);
				addFn(material->m_AO, material->m_Uniform.UseAOTex, index, textures);

				uniforms[i] = material->m_Uniform;
			}
		}

		{
			auto storage = RendererStorage::GetSingleton();
			auto defMaterial = storage->GetDefaultMaterial();

			defMaterial->UpdateSamplers(textures, storage->m_TexturesBinding);
			defMaterial->SubmitBuffer(storage->m_MaterialsBinding, sizeof(PBRUniform) * uniforms.size(), uniforms.data());
		}
	}

	void PBRFactory::AddDefaultMaterial()
	{
		PBRCreateInfo ci{};
		ci.Roughness = 1.0f;
		ci.Metallness = 0.2f;

		auto material = AddMaterial(&ci, "default material");
		material->m_Uniform.ID = 0;
	}

	bool PBRFactory::RemoveMaterial(const std::string& name)
	{
		auto& it = s_Instance->m_IDs.find(name);
		if (it != s_Instance->m_IDs.end())
		{
			auto& pos = std::find(s_Instance->m_Materials.begin(), s_Instance->m_Materials.end(), it->second);

			s_Instance->m_Materials.erase(pos);
			s_Instance->m_IDs.erase(name);

			return true;
		}

		return false;
	}

	bool PBRFactory::IsMaterialExist(const std::string& name)
	{
		return s_Instance->m_IDs.find(name) != s_Instance->m_IDs.end();
	}

	uint32_t PBRFactory::GetMaterialCount()
	{
		return static_cast<uint32_t>(s_Instance->m_Materials.size());
	}

	Ref<PBRHandle> PBRFactory::GetMaterial(const std::string& name)
	{
		auto& it = s_Instance->m_IDs.find(name);
		if (it != s_Instance->m_IDs.end())
			return it->second;

		return nullptr;
	}

	const std::vector<Ref<PBRHandle>>& PBRFactory::GetMaterials()
	{
		return s_Instance->m_Materials;
	}

	void PBRHandle::SetTexture(const Ref<Texture>& tex, PBRTexture type)
	{
		switch (type)
		{
		case PBRTexture::Albedo:
		{
			m_Albedo = tex;
			m_Uniform.UseAlbedroTex = m_Albedo != nullptr;
			break;
		}
		case PBRTexture::Normal:
		{
			m_Normal = tex;
			m_Uniform.UseNormalTex = m_Normal != nullptr;
			break;
		}
		case PBRTexture::Metallic:
		{
			m_Metallness = tex;
			m_Uniform.UseMetallicTex = m_Metallness != nullptr;
			break;
		}
		case PBRTexture::Roughness:
		{
			m_Roughness = tex;
			m_Uniform.UseRoughnessTex = m_Roughness != nullptr;
			break;
		}
		case PBRTexture::AO:
		{
			m_AO = tex;
			m_Uniform.UseAOTex = m_AO != nullptr;
			break;
		}
		case PBRTexture::Emissive:
		{
			m_Emissive = tex;
			m_Uniform.UseEmissiveTex = m_Emissive != nullptr;
			break;
		}
		default: break;
		}
	}

	void PBRHandle::SetRoughness(float value)
	{
		m_Uniform.Roughness = value;
	}

	void PBRHandle::SetMetallness(float value)
	{
		m_Uniform.Metalness = value;
	}

	void PBRHandle::SetEmission(float value)
	{
		m_Uniform.EmissionStrength = value;
	}

	void PBRHandle::SetAlbedo(const glm::vec3& value)
	{
		m_Uniform.Albedro = glm::vec4(value, 1);
	}

	const PBRUniform& PBRHandle::GetUniform() const
	{
		return m_Uniform;
	}

	uint32_t PBRHandle::GetID() const
	{
		return m_Uniform.ID;
	}

	void PBRCreateInfo::SetTexture(PBRTexture type, const TextureCreateInfo* info)
	{
		switch (type)
		{
		case PBRTexture::Albedo:
			AlbedroTex = *info;
			break;
		case PBRTexture::Normal:
			NormalTex = *info;
			break;
		case PBRTexture::Metallic:
			MetallnessTex = *info;
			break;
		case PBRTexture::Roughness:
			RoughnessTex = *info;
			break;
		case PBRTexture::AO:
			AOTex = *info;
			break;
		case PBRTexture::Emissive:
			EmissiveTex = *info;
			break;
		default: break;
		}
	}

	void PBRCreateInfo::GetTextures(std::map<PBRTexture, TextureCreateInfo*>& out_hashmap)
	{
		if (AlbedroTex.FilePath.empty() == false)
		{
			out_hashmap[PBRTexture::Albedo] = &AlbedroTex;
		}

		if (NormalTex.FilePath.empty() == false)
		{
			out_hashmap[PBRTexture::Normal] = &NormalTex;
		}

		if (MetallnessTex.FilePath.empty() == false)
		{
			out_hashmap[PBRTexture::Metallic] = &MetallnessTex;
		}

		if (RoughnessTex.FilePath.empty() == false)
		{
			out_hashmap[PBRTexture::Roughness] = &RoughnessTex;
		}

		if (AOTex.FilePath.empty() == false)
		{
			out_hashmap[PBRTexture::AO] = &AOTex;
		}

		if (EmissiveTex.FilePath.empty() == false)
		{
			out_hashmap[PBRTexture::Emissive] = &EmissiveTex;
		}
	}

	bool PBRCreateInfo::Load(const std::string& filePath)
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
				AOTex, EmissiveTex, Albedo.r, Albedo.g, Albedo.b);
		}

		return true;
	}

	bool PBRCreateInfo::Save(const std::string& filePath)
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