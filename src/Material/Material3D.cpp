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
		MaterialCreateInfo materialCI{};
		ShaderCreateInfo shaderCI = {};

		{
			const auto& path = GraphicsContext::GetSingleton()->GetResourcesPath();
			shaderCI.FilePaths[ShaderType::Vertex] = path + "Shaders/Gbuffer.vert";
			shaderCI.FilePaths[ShaderType::Fragment] = path + "Shaders/Gbuffer.frag";
			// SSBO's
			ShaderBufferInfo bufferInfo = {};

			// Vertex
			bufferInfo.Size = sizeof(InstanceData) * max_objects;
			shaderCI.BufferInfos[storage->m_ShaderDataBinding] = bufferInfo;

			bufferInfo.Size = sizeof(Uniform) * max_materials;
			shaderCI.BufferInfos[storage->m_MaterialsBinding] = bufferInfo;

			bufferInfo.Size = sizeof(glm::mat4) * max_anim_joints;
			shaderCI.BufferInfos[storage->m_AnimBinding] = bufferInfo;
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

		AddDefaultMaterial();
		return Build(&materialCI);
	}

	Ref<Material3D> Material3D::Create()
	{
		return std::make_shared<Material3D>();
	}

	Ref<Material3D::Info> Material3D::AddMaterial(CreateInfo* infoCI, const std::string& name)
	{
		Ref<Info> material = GetMaterial(name);
		if (material != nullptr)
			return material;

		{
			std::hash<std::string_view> hasher{};

			material = std::make_shared<Info>();
			material->Uniform.ID = static_cast<uint32_t>(hasher(name));

			material->Uniform.Metalness = infoCI->Metallness;
			material->Uniform.Roughness = infoCI->Roughness;
			material->Uniform.EmissionStrength = infoCI->EmissionStrength;
			material->Uniform.Albedro = glm::vec4(infoCI->Albedo, 1);

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

			std::vector<Ref<Texture>> textures(m_MaxTextures);
			RendererStorage* storage = dynamic_cast<RendererStorage*>(GetInfo().pStorage);
			UpdateSamplers(textures, storage->m_TexturesBinding);
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

	Ref<Material3D::Info> Material3D::GetMaterial(const std::string& name)
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

	void Material3D::UpdateMaterials()
	{
		std::vector<Uniform> uniforms;
		std::vector<Ref<Texture>> textures(m_MaxTextures);

		constexpr auto addFn = [](const Ref<Texture>& texture, uint32_t& tex_index, uint32_t& global_index, std::vector<Ref<Texture>>& out_textures)
		{
			if (texture != nullptr)
			{
				tex_index = global_index;
				out_textures[global_index] = texture;
				global_index++;
			}
		};

		uint32_t index = 0;
		for (auto& material : m_Materials)
		{
			addFn(material->Albedo, material->Uniform.AlbedroTexIndex, index, textures);
			addFn(material->Normal, material->Uniform.NormalTexIndex, index, textures);
			addFn(material->Roughness, material->Uniform.RoughnessTexIndex, index, textures);
			addFn(material->Metallness, material->Uniform.MetallicTexIndex, index, textures);
			addFn(material->Emissive, material->Uniform.EmissiveTexIndex, index, textures);
			addFn(material->AO, material->Uniform.UseAOTex, index, textures);

			uniforms.emplace_back(material->Uniform);
		}

		RendererStorage* storage = dynamic_cast<RendererStorage*>(GetInfo().pStorage);
		UpdateSamplers(textures, storage->m_TexturesBinding);
		SubmitBuffer(storage->m_MaterialsBinding, sizeof(Uniform) * uniforms.size(), uniforms.data());
	}

	bool Material3D::AddDefaultMaterial()
	{
		CreateInfo ci{};
		ci.Roughness = 1.0f;
		ci.Metallness = 0.2f;

		auto material = AddMaterial(&ci, "default material");
		material->Uniform.ID = 0;

		return true;
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
				AOTex, EmissiveTex, Albedo.r, Albedo.g, Albedo.b);
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

	void Material3D::Info::SetTexture(const Ref<Texture>& tex, TextureType type)
	{
		switch (type)
		{
		case Material3D::TextureType::Albedo:
		{
			Albedo = tex;
			Uniform.UseAlbedroTex = Albedo != nullptr;
			break;
		}
		case Material3D::TextureType::Normal:
		{
			Normal = tex;
			Uniform.UseNormalTex = Normal != nullptr;
			break;
		}
		case Material3D::TextureType::Metallic:
		{
			Metallness = tex;
			Uniform.UseMetallicTex = Metallness != nullptr;
			break;
		}
		case Material3D::TextureType::Roughness:
		{
			Roughness = tex;
			Uniform.UseRoughnessTex = Roughness != nullptr;
			break;
		}
		case Material3D::TextureType::AO:
		{
			AO = tex;
			Uniform.UseAOTex = AO != nullptr;
			break;
		}
		case Material3D::TextureType::Emissive:
		{
			Emissive = tex;
			Uniform.UseEmissiveTex = Emissive != nullptr;
			break;
		}
		default: break;
		}
	}

	void Material3D::Info::SetRoughness(float value)
	{
		Uniform.Roughness = value;
	}

	void Material3D::Info::SetMetallness(float value)
	{
		Uniform.Metalness = value;
	}

	void Material3D::Info::SetEmission(float value)
	{
		Uniform.EmissionStrength = value;
	}

	void Material3D::Info::SetAlbedo(const glm::vec3& value)
	{
		Uniform.Albedro = glm::vec4(value, 1);
	}

	const Material3D::Uniform& Material3D::Info::GetUniform() const
	{
		return Uniform;
	}

	uint32_t Material3D::Info::GetID() const
	{
		return Uniform.ID;
	}
}