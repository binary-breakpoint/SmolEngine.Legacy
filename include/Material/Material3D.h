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
		private:
			uint32_t   Pad1;
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
			glm::vec3          AlbedroColor = glm::vec3(1.0f);

		private:
			friend class cereal::access;

			template<typename Archive>
			void serialize(Archive& archive)
			{
				archive(Metallness, Roughness, EmissionStrength, AlbedroTex, NormalTex, MetallnessTex, RoughnessTex, AOTex,
					EmissiveTex, AlbedroColor.r, AlbedroColor.g, AlbedroColor.b);
			}
		};

		struct MaterialInfo
		{
			Ref<Texture>   Albedo = nullptr;
			Ref<Texture>   Normal = nullptr;
			Ref<Texture>   Metallness = nullptr;
			Ref<Texture>   Roughness = nullptr;
			Ref<Texture>   Emissive = nullptr;
			Ref<Texture>   AO = nullptr;

			const Uniform& GetUniform() const;
			uint32_t       GetID() const;

		private:
			Uniform        Uniform{};
			uint32_t       ID = 0;

			friend class Material3D;
		};

		void                    ClearMaterials();
		bool                    RemoveMaterial(const std::string& name);
		bool                    IsMaterialExist(const std::string& name);
		uint32_t                GetMaterialCount() const;
		Ref<MaterialInfo>       AddMaterial(MaterialCreateInfo* infoCI, const std::string& name);
		Ref<MaterialInfo>       GetMaterial(const std::string& name);
							    
	private:				    
		bool                    LoadAsDefault(RendererStorage* storage);
		static Ref<Material3D>  Create();

	private:
		std::mutex                                         m_Mutex{};
		std::vector<Ref<MaterialInfo>>                     m_Materials;
		std::unordered_map<std::string, Ref<MaterialInfo>> m_IDs;

		friend class DeferredRenderer;
	};
}