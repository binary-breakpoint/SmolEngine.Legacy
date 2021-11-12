#include "stdafx.h"
#include "ECS/Components/Singletons/AudioEngineSComponent.h"

#include <soloud.h>
#include <soloud_fft.h>

namespace SmolEngine
{
	AudioEngineSComponent* AudioEngineSComponent::Instance = nullptr;

	AudioEngineSComponent::AudioEngineSComponent()
	{
		Instance = this;
		if (Core == nullptr)
		{
			Core = new SoLoud::Soloud();
			Core->init(SoLoud::Soloud::ENABLE_VISUALIZATION);
		}
	}

	AudioEngineSComponent::~AudioEngineSComponent()
	{
		Instance = nullptr;
	}
}