#pragma once
#include "Core/Core.h"
#include "Audio/AudioHandle.h"
#include "Audio/AudioSourceFilter.h"

#include <any>

namespace Soloud
{
	class AudioSource;
}

namespace SmolEngine
{
	struct SoloudStorage;

	struct AudioSource
	{
		AudioSource();
		virtual ~AudioSource();

		void               Initialize();
		float*             GetFFT();
		float*             GetWave();

		Ref<AudioHandle>   SourceHandle = nullptr;
		Ref<AudioHandle>   GroupHandle = nullptr;
		Ref<std::any>      Mixer = nullptr;
		float              Volume = 1.0f;
		float              Speed = 1.0f;
		uint32_t           FilterCount = 0;
		AudioSourceFilter  Filter{};
	};
}