#include "stdafx.h"
#include "Pools/AudioPool.h"
#include "Asset/AssetManager.h"

namespace SmolEngine
{
	Ref<AudioClip> AudioPool::ConstructFromFile(AudioClipCreateInfo* info)
	{
		Ref<AudioClip> clip = GetByPath(info->FilePath);
		if (clip) { return clip; }

		clip = std::make_shared<AudioClip>(*info);

		AssetManager::Add(info->FilePath, clip, AssetType::Audio);

		return clip;
	}

	Ref<AudioClip> AudioPool::GetByPath(const std::string& path)
	{
		return AssetManager::GetAsset<AudioClip>(path);
	}
}