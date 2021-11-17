#pragma once
#include "Audio/AudioClip.h"

namespace SmolEngine
{
	class AudioPool
	{
	public:
		static Ref<AudioClip> ConstructFromFile(AudioClipCreateInfo* info);
		static Ref<AudioClip> GetByPath(const std::string& path);
	};
}