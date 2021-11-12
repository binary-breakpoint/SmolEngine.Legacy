#include "stdafx.h"
#include "ECS/AssetManager.h"

#include "ECS/Prefab.h"
#include "Audio/AudioClip.h"

#include "Primitives/Texture.h"
#include "Primitives/Mesh.h"

namespace SmolEngine
{
	void AssetManager::OnReset()
	{
		m_MeshMap.clear();
		m_TextureMap.clear();
		m_PrefabMap.clear();
		m_AudioClipMap.clear();
		m_HashMap.clear();
	}

	void AssetManager::AddPath(const std::string& path, const size_t& id)
	{
		auto& it = m_HashMap.find(id);
		if (it == m_HashMap.end())
		{
			m_HashMap[id] = path;
		}
	}

	size_t AssetManager::AddMesh(const std::string& filePath, Ref<Mesh>& out_object)
	{
		size_t id = m_Hasher(filePath);
		auto& it = m_MeshMap.find(id);
		if (it != m_MeshMap.end())
		{
			out_object = it->second;
		}
		else
		{
			out_object = std::make_shared<Mesh>();
			Mesh::Create(filePath, out_object.get());
			{
				std::lock_guard<std::mutex> lock(m_Mutex);
				m_MeshMap[id] = out_object;
				AddPath(filePath, id);
			}
		}

		return id;
	}

	size_t AssetManager::AddTexture(const std::string& filePath, Ref<Texture>& out_object)
	{
		size_t id = m_Hasher(filePath);
		auto& it = m_TextureMap.find(id);
		if (it != m_TextureMap.end())
		{
			out_object = it->second;
		}
		else
		{
			out_object = std::make_shared<Texture>();
			TextureCreateInfo info;
			if (info.Load(filePath))
			{
				Texture::Create(&info, out_object.get());
				{
					std::lock_guard<std::mutex> lock(m_Mutex);
					m_TextureMap[id] = out_object;
					AddPath(filePath, id);
				}
			}
		}

		return id;
	}

	size_t AssetManager::AddPrefab(const std::string& filePath, Ref<Prefab>& out_object)
	{
		size_t id = m_Hasher(filePath);
		auto& it = m_PrefabMap.find(id);
		if (it != m_PrefabMap.end())
		{
			out_object = it->second;
		}
		else
		{
			out_object = std::make_shared<Prefab>();
			out_object->LoadFromFile(filePath);
			{
				std::lock_guard<std::mutex> lock (m_Mutex);
				m_PrefabMap[id] = out_object;
				AddPath(filePath, id);
			}
		}

		return id;
	}

	size_t AssetManager::AddAudioClip(AudioClipCreateInfo* createInfo, Ref<AudioClip>& out_object)
	{
		size_t id = m_Hasher(createInfo->FilePath);
		auto& it = m_AudioClipMap.find(id);
		if (it != m_AudioClipMap.end())
		{
			out_object = it->second;
			createInfo->FileName = it->second->GetCretaeInfo().FileName;
		}
		else
		{
			out_object = std::make_shared<AudioClip>(*createInfo);
			{
				std::lock_guard<std::mutex> lock(m_Mutex);
				m_AudioClipMap[id] = out_object;
			}
		}

		return id;
	}

	Ref<Mesh> AssetManager::GetMesh(const size_t& id)
	{
		auto& it = m_MeshMap.find(id);
		if (it != m_MeshMap.end())
			return it->second;

		return nullptr;
	}

	Ref<Texture> AssetManager::GetTexture(const size_t& id)
	{
		auto& it = m_TextureMap.find(id);
		if (it != m_TextureMap.end())
			return it->second;

		return nullptr;
	}

	Ref<Prefab> AssetManager::GetPrefab(const size_t& id)
	{
		auto& it = m_PrefabMap.find(id);
		if (it != m_PrefabMap.end())
			return it->second;

		return nullptr;
	}

	Ref<AudioClip> AssetManager::GetAudioClip(const size_t& id)
	{
		auto& it = m_AudioClipMap.find(id);
		if (it != m_AudioClipMap.end())
			return it->second;

		return nullptr;
	}

	bool AssetManager::GetPath(const size_t& id, std::string& out_path)
	{
		auto& it = m_HashMap.find(id);
		if (it != m_HashMap.end())
		{
			out_path = it->second;
			return true;
		}

		return false;
	}
}