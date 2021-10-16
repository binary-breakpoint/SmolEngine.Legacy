#pragma once
#include "Material/Material3D.h"

#include <mutex>

namespace cereal
{
	class access;
}

namespace SmolEngine
{
	struct RendererStorage;

	enum class PBRTexture : uint32_t
	{
		Albedo,
		Normal,
		Metallic,
		Roughness,
		AO,
		Emissive,
	};

	struct PBRMaterialUniform
	{
		glm::vec4  Albedro;
		float      Metalness = 0.2f;
		float      Roughness = 1.0f;
		float      EmissionStrength = 1.0f;
		uint32_t   UseAlbedroTex = 0;
		uint32_t   UseNormalTex = 0;
		uint32_t   UseMetallicTex = 0;
		uint32_t   UseRoughnessTex = 0;
		uint32_t   UseAOTex = 0;
		uint32_t   UseEmissiveTex = 0;
		uint32_t   AlbedroTexIndex = 0;
		uint32_t   NormalTexIndex = 0;
		uint32_t   MetallicTexIndex = 0;
		uint32_t   RoughnessTexIndex = 0;
		uint32_t   AOTexIndex = 0;
		uint32_t   EmissiveTexIndex = 0;
		uint32_t   ID = 0;
	};

	struct PBRMaterialCreateInfo
	{
		void               SetTexture(PBRTexture type, const TextureCreateInfo* info);
		void               GetTextures(std::unordered_map<PBRTexture, TextureCreateInfo*>& out_hashmap);
		bool               Load(const std::string& filePath);
		bool               Save(const std::string& filePath);

		bool               Used = false;
		float              Metallness = 0.2f;
		float              Roughness = 1.0f;
		float              EmissionStrength = 1.0f;
		TextureCreateInfo  AlbedroTex = {};
		TextureCreateInfo  NormalTex = {};
		TextureCreateInfo  MetallnessTex = {};
		TextureCreateInfo  RoughnessTex = {};
		TextureCreateInfo  AOTex = {};
		TextureCreateInfo  EmissiveTex = {};
		glm::vec3          Albedo = glm::vec3(1.0f);

	private:
		friend class cereal::access;

		template<typename Archive>
		void serialize(Archive& archive)
		{
			archive(Metallness, Roughness, EmissionStrength, AlbedroTex, NormalTex, MetallnessTex, RoughnessTex, AOTex,
				EmissiveTex, Albedo.r, Albedo.g, Albedo.b);
		}
	};

	struct PBRMaterialHandle
	{
		void                      SetTexture(const Ref<Texture>& tex, PBRTexture type);
		void                      SetRoughness(float value);
		void                      SetMetallness(float value);
		void                      SetEmission(float value);
		void                      SetAlbedo(const glm::vec3& value);
		const PBRMaterialUniform& GetUniform() const;
		uint32_t                  GetID() const;

	private:
		Ref<Texture>             m_Albedo = nullptr;
		Ref<Texture>             m_Normal = nullptr;
		Ref<Texture>             m_Metallness = nullptr;
		Ref<Texture>             m_Roughness = nullptr;
		Ref<Texture>             m_Emissive = nullptr;
		Ref<Texture>             m_AO = nullptr;
		PBRMaterialUniform       m_Uniform{};

		friend class MaterialPBR;
	};

	class MaterialPBR : public Material3D
	{
	public:
		void                                       ClearMaterials();
		void                                       UpdateMaterials();
		bool                                       AddDefaultMaterial();
		bool                                       RemoveMaterial(const std::string& name);
		bool                                       IsMaterialExist(const std::string& name);

		uint32_t                                   GetMaterialCount() const;
		Ref<PBRMaterialHandle>                     AddMaterial(PBRMaterialCreateInfo* infoCI, const std::string& name);
		Ref<PBRMaterialHandle>                     GetMaterial(const std::string& name);
		const std::vector<Ref<PBRMaterialHandle>>& GetMaterials() const;
							    
	private:				    
		virtual void                               OnPushConstant(const uint32_t& dataOffset) override;
		virtual void                               OnDrawCommand(RendererDrawCommand* command) override;

		static Ref<MaterialPBR>                    Create();
		bool                                       Initialize(RendererStorage* storage);

	private:
		std::unordered_map<std::string, Ref<PBRMaterialHandle>> m_IDs;
		std::vector<Ref<PBRMaterialHandle>> m_Materials;
		const uint32_t m_MaxTextures = 4096;
		std::mutex m_Mutex{};

		friend struct RendererStorage;
	};
}