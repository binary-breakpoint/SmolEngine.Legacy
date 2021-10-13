#pragma once
#include "Material/Material.h"

#include <mutex>

namespace cereal
{
	class access;
}

namespace SmolEngine
{
	struct RendererStorage;

	class Material3D : public Material
	{
	public:
		enum class TextureType : uint32_t
		{
			Albedo,
			Normal,
			Metallic,
			Roughness,
			AO,
			Emissive,
		};

		struct Uniform
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

		struct CreateInfo
		{
			void               SetTexture(Material3D::TextureType type, const TextureCreateInfo* info);
			void               GetTextures(std::unordered_map<Material3D::TextureType, TextureCreateInfo*>& out_hashmap);
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

		struct Info
		{
			void           SetTexture(const Ref<Texture>& tex, TextureType type);
			void           SetRoughness(float value);
			void           SetMetallness(float value);
			void           SetEmission(float value);
			void           SetAlbedo(const glm::vec3& value);
			const Uniform& GetUniform() const;
			uint32_t       GetID() const;

		private:
			Ref<Texture>   Albedo = nullptr;
			Ref<Texture>   Normal = nullptr;
			Ref<Texture>   Metallness = nullptr;
			Ref<Texture>   Roughness = nullptr;
			Ref<Texture>   Emissive = nullptr;
			Ref<Texture>   AO = nullptr;
			Uniform        Uniform{};

			friend class Material3D;
		};

		void                    ClearMaterials();
		void                    UpdateMaterials();
		bool                    AddDefaultMaterial();
		bool                    RemoveMaterial(const std::string& name);
		bool                    IsMaterialExist(const std::string& name);
		uint32_t                GetMaterialCount() const;
		Ref<Info>               AddMaterial(CreateInfo* infoCI, const std::string& name);
		Ref<Info>               GetMaterial(const std::string& name);
							    
	private:				    
		bool                    LoadAsDefault(RendererStorage* storage);
		static Ref<Material3D>  Create();

	private:
		const uint32_t                             m_MaxTextures = 4096;
		std::mutex                                 m_Mutex{};
		std::vector<Ref<Info>>                     m_Materials;
		std::unordered_map<std::string, Ref<Info>> m_IDs;

		friend struct RendererStorage;
	};
}