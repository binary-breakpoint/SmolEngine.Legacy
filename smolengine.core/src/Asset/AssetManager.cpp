#include "Asset/AssetManager.h"

namespace SmolEngine
{
	AssetManager* AssetManager::s_Instance = new AssetManager();

	AssetManager::AssetManager()
	{
		s_Instance = this;
	}

	void AssetManager::Add(const std::string& path, const Ref<Asset>& asset, AssetType type)
	{
		auto hash = std::hash<std::string>{}(path);

		std::lock_guard<std::mutex> lock(s_Instance->m_Mutex);

		asset->m_UUID = hash;
		asset->m_Type = type;
		asset->m_Flags = AssetFlag::None;

		s_Instance->m_IDHash[path] = hash;
		s_Instance->m_PathHash[hash] = path;
		s_Instance->m_Registry[hash] = asset;
	}

	bool AssetManager::Contains(const std::string& path)
	{
		return s_Instance->m_IDHash.find(path) != s_Instance->m_IDHash.end();
	}

	bool AssetManager::Remove(const std::string& path)
	{
		const auto& it = s_Instance->m_IDHash.find(path);
		if (it != s_Instance->m_IDHash.end())
		{
			{
				std::lock_guard<std::mutex> lock(s_Instance->m_Mutex);

				s_Instance->m_Registry.erase(it->second);
				s_Instance->m_PathHash.erase(it->second);
				s_Instance->m_IDHash.erase(path);
			}

			return true;
		}

		return false;
	}

	std::string AssetManager::GetPathByID(size_t id)
	{
		const auto& it = s_Instance->m_PathHash.find(id);
		if (it != s_Instance->m_PathHash.end())
			return it->second;

		return "";
	}

	void AssetManager::Clear()
	{
		s_Instance->m_Registry.clear();
		s_Instance->m_PathHash.clear();
		s_Instance->m_IDHash.clear();
	}

	uint32_t AssetManager::GetCount()
	{
		return static_cast<uint32_t>(s_Instance->m_Registry.size());
	}
}