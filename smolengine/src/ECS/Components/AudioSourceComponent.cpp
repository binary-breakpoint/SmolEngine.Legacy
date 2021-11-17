#include "stdafx.h"
#include "ECS/Components/AudioSourceComponent.h"
#include "ECS/Components/Singletons/AudioEngineSComponent.h"
#include "ECS/Systems/AudioSystem.h"
#include "Pools/AudioPool.h"

#include <soloud.h>
#include <soloud_speech.h>
#include <soloud_wav.h>
#include <soloud_thread.h>
#include <soloud_bus.h>
#include <soloud_biquadresonantfilter.h>
#include <soloud_echofilter.h>
#include <soloud_dcremovalfilter.h>
#include <soloud_bassboostfilter.h>
#include <soloud_lofifilter.h>
#include <soloud_freeverbfilter.h>
#include <soloud_waveshaperfilter.h>


namespace SmolEngine
{
	AudioSourceComponent::AudioSourceComponent()
		:EnginePtr(AudioEngineSComponent::Get()) {}

	AudioSourceComponent::AudioSourceComponent(uint32_t id)
		: BaseComponent(id), EnginePtr(AudioEngineSComponent::Get()) {}

	bool AudioSourceComponent::PlayClip(Ref<AudioClip>& sound, Ref<AudioHandle>& out_handle, bool add_to_mixer)
	{
		Ref<AudioHandle> handle = nullptr;

		if (sound->IsValid())
		{
			uint32_t id = 0;
			auto core = EnginePtr->Core;
			const auto& info = sound->m_CreateInfo;

			switch (info.eType)
			{
			case ClipType::Sound_2D:
			{
				id = core->play(*sound->m_Obj, info.Volume);

				break;
			}
			case ClipType::Sound_3D:
			{
				id = core->play3d(*sound->m_Obj, info.WorldPos.x, info.WorldPos.y, info.WorldPos.z, info.Velocity.x, info.Velocity.y, info.Velocity.z, info.Volume);

				break;
			}
			case ClipType::Sound_Background:
			{
				id = core->playBackground(*sound->m_Obj, info.Volume);
				break;
			}
			default: break;
			}

			if (id > 0)
			{
				core->setPause(id, true);
				{
					if (info.bLoop)
					{
						core->setLooping(id, info.bLoop);
						core->setLoopPoint(id, info.LoopTime);
					}

					switch (info.eSampleRate)
					{
					case ClipSampleRate::HZ_22050: core->setSamplerate(id, 22050.0f); break;
					case ClipSampleRate::HZ_4410: core->setSamplerate(id, 4410.0f); break;
					case ClipSampleRate::HZ_4800: core->setSamplerate(id, 4800.0f); break;
					case ClipSampleRate::HZ_8000: core->setSamplerate(id, 8000.0f); break;
					}

					core->setRelativePlaySpeed(id, info.Speed * 10.0f);
					core->addVoiceToGroup(*GroupHandle, id);
				}
				core->setPause(id, false);
				if (add_to_mixer)
				{
					auto mixer = Mixer->_Cast<SoLoud::Bus>();
					if (mixer)
					{
						mixer->annexSound(id);
					}
				}

				EnginePtr->Handles.emplace_back(std::make_shared<AudioHandle>(id));
				handle = EnginePtr->Handles.back();
			}
		}

		out_handle = handle;
		return handle != nullptr;
	}

	bool AudioSourceComponent::StopClip(const Ref<AudioHandle>& handle)
	{
		uint32_t id = handle->m_Handle;
		auto core = EnginePtr->Core;

		if (core->isValidVoiceHandle(id))
		{
			core->stop(id);
			AudioSystem::RemoveHandle(id);
			return true;
		}

		handle->m_Handle = 0;
		return false;
	}

	bool AudioSourceComponent::PauseClip(const Ref<AudioHandle>& handle, bool pause)
	{
		uint32_t id = handle->m_Handle;
		auto core = EnginePtr->Core;

		if (core->isValidVoiceHandle(id))
		{
			core->setPause(id, pause);
			return true;
		}

		handle->m_Handle = 0;
		return false;
	}

	bool AudioSourceComponent::SetLooping(const Ref<AudioHandle>& handle, bool value, float timePoint)
	{
		return false;
	}

	bool AudioSourceComponent::SetSpeed(const Ref<AudioHandle>& handle, float value)
	{
		return false;
	}

	bool AudioSourceComponent::SetSampleRate(const Ref<AudioHandle>& handle, ClipSampleRate rate)
	{
		return false;
	}

	bool AudioSourceComponent::SetPosition(const Ref<AudioHandle>& handle, const glm::vec3& postion)
	{
		return false;
	}

	void AudioSourceComponent::SetVolume(float value)
	{
		EnginePtr->Core->setVolume(*GroupHandle, value);
	}

	void AudioSourceComponent::SetSpeed(float value)
	{
		EnginePtr->Core->setRelativePlaySpeed(*GroupHandle, value * 10.0f);
	}

	void AudioSourceComponent::AddFilter(AudioFilterFlags filter)
	{
		Filter.Flags |= filter;
	}

	void AudioSourceComponent::RemoveFilter(AudioFilterFlags filter)
	{
		Filter.Flags &= ~filter;
	}

	bool AudioSourceComponent::RemoveClipAtIndex(uint32_t index)
	{
		size_t old_size = Clips.size();
		Clips.erase(Clips.begin() + index);

		return old_size != Clips.size();
	}

	void AudioSourceComponent::CreateFiltersEX()
	{
		SoLoud::Bus* mixer = Mixer->_Cast<SoLoud::Bus>();
		AudioSourceFilter& ASfilter = Filter;

		if ((ASfilter.Flags & AudioFilterFlags::Echo) == AudioFilterFlags::Echo)
		{
			SoLoud::EchoFilter filter;
			filter.setParams(ASfilter.EchoCI.Delay, ASfilter.EchoCI.Decay, ASfilter.EchoCI.Filter);
			mixer->setFilter(FilterCount, &filter);
			FilterCount++;
		}

		if ((ASfilter.Flags & AudioFilterFlags::Bassboost) == AudioFilterFlags::Bassboost)
		{
			SoLoud::BassboostFilter filter;
			filter.setParams(ASfilter.BassboostCI.Boost);
			mixer->setFilter(FilterCount, &filter);
			FilterCount++;
		}

		if ((ASfilter.Flags & AudioFilterFlags::BQResonant) == AudioFilterFlags::BQResonant)
		{
			SoLoud::BiquadResonantFilter filter;
			filter.setParams(ASfilter.BQResonantCI.Type, ASfilter.BQResonantCI.Frequency, ASfilter.BQResonantCI.Resonance);
			mixer->setFilter(FilterCount, &filter);
			FilterCount++;
		}

		if ((ASfilter.Flags & AudioFilterFlags::Freeverb) == AudioFilterFlags::Freeverb)
		{
			SoLoud::FreeverbFilter filter;
			filter.setParams(ASfilter.FreeverbCI.Mode, ASfilter.FreeverbCI.RoomSize, ASfilter.FreeverbCI.Damp, ASfilter.FreeverbCI.Width);
			mixer->setFilter(FilterCount, &filter);
			FilterCount++;
		}

		if ((ASfilter.Flags & AudioFilterFlags::Lofi) == AudioFilterFlags::Lofi)
		{
			SoLoud::LofiFilter filter;
			filter.setParams(ASfilter.LofiCI.SampleRate, ASfilter.LofiCI.Bitdepth);
			mixer->setFilter(FilterCount, &filter);
			FilterCount++;
		}

		if ((ASfilter.Flags & AudioFilterFlags::WaveShaper) == AudioFilterFlags::WaveShaper)
		{
			SoLoud::WaveShaperFilter filter;
			filter.setParams(ASfilter.WaveShaperCI.Amount);
			mixer->setFilter(FilterCount, &filter);
			FilterCount++;
		}
	}

	void AudioSourceComponent::ClearFiltersEX()
	{
	}

	void AudioSourceComponent::StopAll()
	{
		SoLoud::Bus* mixer = Mixer->_Cast<SoLoud::Bus>();
		if (mixer->getActiveVoiceCount() > 0)
		{
			AudioEngineSComponent::Get()->Core->stop(*GroupHandle);
		}

		ClearFiltersEX();
	}

	void AudioSourceComponent::PlayAll()
	{
		for (auto& clip : Clips)
		{
			CreateFiltersEX();
			PlayClip(clip.m_Clip, clip.m_Handle);
		}
	}

	void AudioSourceComponent::LoadClip(const std::string& path)
	{
		AudioClipCreateInfo info{};
		if (info.Load(path))
		{
			LoadClip(&info);
		}
	}

	void AudioSourceComponent::LoadClip(AudioClipCreateInfo* info)
	{
		Clips.emplace_back(ClipInstance());
		auto& clip = Clips.back();
		clip.m_CreateInfo = *info;

		clip.m_Clip = AudioPool::ConstructFromFile(&clip.m_CreateInfo);
		if (clip.m_CreateInfo.bPlayOnAwake && WorldAdmin::GetSingleton()->IsInPlayMode())
		{
			PlayClip(clip.m_Clip, clip.m_Handle);
		}
	}

	ClipInstance* AudioSourceComponent::GetClipByIndex(uint32_t index)
	{
		return &Clips[index];
	}
}