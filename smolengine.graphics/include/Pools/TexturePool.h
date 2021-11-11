#pragma once
#include "Memory.h"

#include <unordered_map>
#include <mutex>

namespace SmolEngine
{
	class Texture;
	struct TextureCreateInfo;

	class TexturePool
	{
	public:
		TexturePool();
		~TexturePool();

		static Ref<Texture> GetWhiteTexture();
		static Ref<Texture> GetStorageTexture();
		static Ref<Texture> GetCubeMap();

		static Ref<Texture> GetByPath(const std::string& path);
		static Ref<Texture> GetByID(size_t id);

		static Ref<Texture> ConstructFromFile(TextureCreateInfo* texCI);
		static Ref<Texture> ConstructFromPath(const std::string& path);
		static bool         Remove(size_t id);
		static void         Clear();


	private:
		inline static TexturePool*               s_Instance = nullptr;
		Ref<Texture>                             m_WhiteTexture = nullptr;
		Ref<Texture>                             m_StorageTexure = nullptr;
		Ref<Texture>                             m_CubeMap = nullptr;
		std::mutex                               m_Mutex{};
		std::unordered_map<size_t, Ref<Texture>> m_IDMap;
	};
}