#include "stdafx.h"
#include "Materials/MaterialPBR.h"
#include "Renderer/RendererDeferred.h"

#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/archives/json.hpp>

namespace SmolEngine
{
	bool MaterialPBR::Initialize(RendererStorage* storage)
	{
		GraphicsPipelineCreateInfo pipelineCI = {};
		MaterialCreateInfo materialCI{};
		ShaderCreateInfo shaderCI = {};

		{
			const auto& path = GraphicsContext::GetSingleton()->GetResourcesPath();

			shaderCI.Stages[ShaderType::Vertex] = path + "Shaders/Gbuffer.vert";
			shaderCI.Stages[ShaderType::Fragment] = path + "Shaders/Gbuffer.frag";
			// SSBO's
			ShaderBufferInfo bufferInfo = {};

			// Vertex
			bufferInfo.Size = sizeof(InstanceData) * max_objects;
			shaderCI.Buffers[storage->m_ShaderDataBinding] = bufferInfo;

			bufferInfo.Size = sizeof(PBRMaterialUniform) * max_materials;
			shaderCI.Buffers[storage->m_MaterialsBinding] = bufferInfo;

			bufferInfo.Size = sizeof(glm::mat4) * max_anim_joints;
			shaderCI.Buffers[storage->m_AnimBinding] = bufferInfo;
		}

		{
			pipelineCI.VertexInputInfos = { GetVertexInputInfo() };
			pipelineCI.PipelineName = "PBR";
			pipelineCI.ShaderCreateInfo = shaderCI;
		}

		materialCI.Name = pipelineCI.PipelineName;
		materialCI.PipelineCreateInfo = pipelineCI;
		materialCI.pStorage = storage;

		AddDefaultMaterial();
		return Build(&materialCI);
	}

	Ref<MaterialPBR> MaterialPBR::Create()
	{
		return std::make_shared<MaterialPBR>();
	}

	Ref<PBRMaterialHandle> MaterialPBR::AddMaterial(PBRMaterialCreateInfo* infoCI, const std::string& name)
	{
		Ref<PBRMaterialHandle> material = GetMaterial(name);
		if (material != nullptr)
			return material;

		{
			std::hash<std::string_view> hasher{};

			material = std::make_shared<PBRMaterialHandle>();
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

		m_Mutex.lock();
		{
			m_Materials.emplace_back(material);
			m_IDs[name] = material;
		}
		m_Mutex.unlock();
		return material;
	}

	bool MaterialPBR::RemoveMaterial(const std::string& name)
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

	bool MaterialPBR::IsMaterialExist(const std::string& name)
	{
		return m_IDs.find(name) != m_IDs.end();
	}

	uint32_t MaterialPBR::GetMaterialCount() const
	{
		return static_cast<uint32_t>(m_Materials.size());
	}

	Ref<PBRMaterialHandle> MaterialPBR::GetMaterial(const std::string& name)
	{
		auto& it = m_IDs.find(name);
		if (it != m_IDs.end())
			return it->second;

		return nullptr;
	}

	const std::vector<Ref<PBRMaterialHandle>>& MaterialPBR::GetMaterials() const
	{
		return m_Materials;
	}

	void MaterialPBR::OnPushConstant(const uint32_t& dataOffset)
	{
		SubmitPushConstant(ShaderType::Vertex, sizeof(uint32_t), &dataOffset);
	}

	void MaterialPBR::OnDrawCommand(RendererDrawCommand* command)
	{
		DrawMeshIndexed(command->Mesh, command->InstancesCount);
	}

	void MaterialPBR::ClearMaterials()
	{
		m_Materials.clear();
		m_IDs.clear();
	}

	void MaterialPBR::UpdateMaterials()
	{
		std::vector<PBRMaterialUniform> uniforms(m_Materials.size());
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

		{
			uint32_t index = 0;

			for (uint32_t i = 0; i < static_cast<uint32_t>(m_Materials.size()); ++i)
			{
				const Ref<PBRMaterialHandle>& material = m_Materials[i];

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
			RendererStorage* storage = dynamic_cast<RendererStorage*>(GetInfo().pStorage);

			UpdateSamplers(textures, storage->m_TexturesBinding);
			SubmitBuffer(storage->m_MaterialsBinding, sizeof(PBRMaterialUniform) * uniforms.size(), uniforms.data());
		}
	}

	bool MaterialPBR::AddDefaultMaterial()
	{
		PBRMaterialCreateInfo ci{};
		ci.Roughness = 1.0f;
		ci.Metallness = 0.2f;

		auto material = AddMaterial(&ci, "default material");
		material->m_Uniform.ID = 0;

		return true;
	}

	void PBRMaterialCreateInfo::SetTexture(PBRTexture type, const TextureCreateInfo* info)
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

	void PBRMaterialCreateInfo::GetTextures(std::unordered_map<PBRTexture, TextureCreateInfo*>& out_hashmap)
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

	bool PBRMaterialCreateInfo::Load(const std::string& filePath)
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

	bool PBRMaterialCreateInfo::Save(const std::string& filePath)
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

	void PBRMaterialHandle::SetTexture(const Ref<Texture>&tex, PBRTexture type)
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

	void PBRMaterialHandle::SetRoughness(float value)
	{
		m_Uniform.Roughness = value;
	}

	void PBRMaterialHandle::SetMetallness(float value)
	{
		m_Uniform.Metalness = value;
	}

	void PBRMaterialHandle::SetEmission(float value)
	{
		m_Uniform.EmissionStrength = value;
	}

	void PBRMaterialHandle::SetAlbedo(const glm::vec3& value)
	{
		m_Uniform.Albedro = glm::vec4(value, 1);
	}

	const PBRMaterialUniform& PBRMaterialHandle::GetUniform() const
	{
		return m_Uniform;
	}

	uint32_t PBRMaterialHandle::GetID() const
	{
		return m_Uniform.ID;
	}
}