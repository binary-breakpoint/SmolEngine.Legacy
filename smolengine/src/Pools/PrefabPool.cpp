#include "stdafx.h"
#include "Pools\PrefabPool.h"
#include "Asset/AssetManager.h"

namespace SmolEngine
{
	Ref<Prefab> PrefabPool::ConstructFromPath(const std::string& path)
	{
		Ref<Prefab> prefab = GetByPath(path);
		if (prefab) { return prefab; }

		prefab = std::make_shared<Prefab>();
		prefab->LoadFromFile(path);

		AssetManager::Add(path, prefab, AssetType::Prefab);

		return prefab;
	}

	Ref<Prefab> PrefabPool::GetByPath(const std::string& path)
	{
		return AssetManager::GetAsset<Prefab>(path);;
	}
}