#include "stdafx.h"
#include "Graphics/Pools/TexturePool.h"

namespace SmolEngine
{
	TexturePool::TexturePool()
	{
		s_Instance = this;

		TextureCreateInfo texCI;
		texCI.Width = 4;
		texCI.Height = 4;

		m_WhiteTexture = Texture::Create();
		m_WhiteTexture->LoadAsWhite();

		m_StorageTexure = Texture::Create();
		m_StorageTexure->LoadAsStorage(&texCI);

		m_CubeMap = Texture::Create();
		m_CubeMap->LoadAsWhiteCube(&texCI);
	}

	TexturePool::~TexturePool()
	{
		s_Instance = nullptr;
	}

	Ref<Texture> TexturePool::GetWhiteTexture()
	{
		return s_Instance->m_WhiteTexture;
	}

	Ref<Texture> TexturePool::GetStorageTexture()
	{
		return s_Instance->m_StorageTexure;
	}

	Ref<Texture> TexturePool::GetCubeMap()
	{
		return s_Instance->m_CubeMap;
	}

	Ref<Texture> TexturePool::GetByPath(const std::string& path)
	{
		std::hash<std::string_view> hasher{};
		size_t id = hasher(path);

		return GetByID(id);
	}

	Ref<Texture> TexturePool::GetByID(size_t id)
	{
		auto& it = s_Instance->m_IDMap.find(id);
		if (it != s_Instance->m_IDMap.end())
		{
			return it->second;
		}

		return nullptr;
	}

	Ref<Texture> TexturePool::ConstructFromFile(TextureCreateInfo* texCI)
	{
		std::hash<std::string_view> hasher{};
		size_t id = hasher(texCI->FilePath);

		auto& it = s_Instance->m_IDMap.find(id);
		if (it != s_Instance->m_IDMap.end())
		{
			return it->second;
		}

		Ref<Texture> texture = Texture::Create();
		texture->LoadFromFile(texCI);

		bool is_loaded = texture->IsGood();
		if (is_loaded)
		{
			s_Instance->m_Mutex.lock();
			s_Instance->m_IDMap[id] = texture;
			s_Instance->m_Mutex.unlock();
		}

		return is_loaded ? texture : nullptr;
	}

	Ref<Texture> TexturePool::ConstructFromPath(const std::string& path)
	{
		TextureCreateInfo texCI = {};
		texCI.FilePath = path;

		return ConstructFromFile(&texCI);
	}

	bool TexturePool::Remove(size_t id)
	{
		return s_Instance->m_IDMap.erase(id);
	}

	void TexturePool::Clear()
	{
		s_Instance->m_IDMap.clear();
	}
}