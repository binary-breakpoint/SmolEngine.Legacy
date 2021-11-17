#include "stdafx.h"
#include "Pools/TexturePool.h"
#include "Asset/AssetManager.h"

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
		return AssetManager::GetAsset<Texture>(path);
	}

	Ref<Texture> TexturePool::ConstructFromFile(TextureCreateInfo* texCI)
	{
		Ref<Texture> texture = GetByPath(texCI->FilePath);
		if (texture) { return texture; }

		texture = Texture::Create();
		texture->LoadFromFile(texCI);

		bool is_loaded = texture->IsGood();
		if (is_loaded)
			AssetManager::Add(texCI->FilePath, texture, AssetType::Texture);

		return is_loaded ? texture : nullptr;
	}

	Ref<Texture> TexturePool::ConstructFromPath(const std::string& path)
	{
		TextureCreateInfo texCI = {};
		texCI.FilePath = path;

		return ConstructFromFile(&texCI);
	}
}