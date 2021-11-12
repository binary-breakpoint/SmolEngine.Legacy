#include "stdafx.h"
#include "Audio/AudioSource.h"
#include "ECS/Components/Singletons/AudioEngineSComponent.h"

#include <soloud.h>
#include <soloud_bus.h>

namespace SmolEngine
{
	AudioSource::AudioSource()
	{

	}

	AudioSource::~AudioSource()
	{
		if (Mixer != nullptr)
		{
			AudioEngineSComponent* engine = AudioEngineSComponent::Get();
			SoLoud::Bus* bus = Mixer->_Cast<SoLoud::Bus>();

			engine->Core->destroyVoiceGroup(*GroupHandle);
			bus->stop();

			Mixer.reset();
			Mixer = nullptr;
		}
	}

	void AudioSource::Initialize()
	{
		if (Mixer == nullptr)
		{
			Mixer = std::make_shared<std::any>(std::make_any<SoLoud::Bus>());

			SoLoud::Bus* bus = Mixer->_Cast<SoLoud::Bus>();
			AudioEngineSComponent* engine = AudioEngineSComponent::Get();

			uint32_t source_id = engine->Core->play(*bus, Volume);
			uint32_t group_id = engine->Core->createVoiceGroup();

			bus->setVisualizationEnable(true);
			SourceHandle = std::make_shared<AudioHandle>(source_id);
			GroupHandle = std::make_shared<AudioHandle>(group_id);
		}
	}

	float* AudioSource::GetFFT()
	{
		SoLoud::Bus* bus = Mixer->_Cast<SoLoud::Bus>();
		return bus->calcFFT();
	}

	float* AudioSource::GetWave()
	{
		SoLoud::Bus* bus = Mixer->_Cast<SoLoud::Bus>();
		return bus->getWave();
	}
}