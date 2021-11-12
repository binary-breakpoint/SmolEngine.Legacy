#include "stdafx.h"
#include "ECS/Systems/AudioSystem.h"

#include "ECS/ComponentsCore.h"
#include "ECS/Components/Singletons/WorldAdminStateSComponent.h"
#include "ECS/Components/Singletons/AudioEngineSComponent.h"

#include "Audio/AudioSource.h"

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
	bool AudioSystem::PlayClip(AudioSource* source, Ref<AudioClip>& sound, Ref<AudioHandle>& out_handle, bool add_to_mixer)
	{
		Ref<AudioHandle> handle = nullptr;

		if (sound->IsValid())
		{
			uint32_t id = 0;
			auto core = m_State->Core;
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
					core->addVoiceToGroup(*source->GroupHandle, id);
				}
				core->setPause(id, false);
				if (add_to_mixer)
				{
					auto mixer = source->Mixer->_Cast<SoLoud::Bus>();
					if (mixer)
					{
						mixer->annexSound(id);
					}
				}

				m_State->Handles.emplace_back(std::make_shared<AudioHandle>(id));
				handle = m_State->Handles.back();
			}
		}

		out_handle = handle;
		return handle != nullptr;
	}

	bool AudioSystem::StopClip(AudioSource* source, const Ref<AudioHandle>& handle)
	{
		uint32_t id = handle->m_Handle;
		auto core = m_State->Core;

		if (core->isValidVoiceHandle(id))
		{
			core->stop(id);
			RemoveHandle(id);
			return true;
		}

		handle->m_Handle = 0;
		return false;
	}

	bool AudioSystem::PauseClip(AudioSource* source, const Ref<AudioHandle>& handle, bool pause)
	{
		uint32_t id = handle->m_Handle;
		auto core = m_State->Core;

		if (core->isValidVoiceHandle(id))
		{
			core->setPause(id, pause);
			return true;
		}

		handle->m_Handle = 0;
		return false;
	}

	bool AudioSystem::SetLooping(AudioSource* source, const Ref<AudioHandle>& handle, bool value, float timePoint)
	{

		return false;
	}

	bool AudioSystem::SetSpeed(AudioSource* source, const Ref<AudioHandle>& handle, float value)
	{
		return false;
	}

	bool AudioSystem::SetSampleRate(AudioSource* source, const Ref<AudioHandle>& handle, ClipSampleRate rate)
	{
		return false;
	}

	bool AudioSystem::Set3DPosition(AudioSource* source, const Ref<AudioHandle>& handle, const glm::vec3& postion)
	{
		return false;
	}

	bool AudioSystem::RemoveClipAtIndex(AudioSourceComponent* comp, uint32_t index)
	{
		size_t old_size = comp->Clips.size();
		comp->Clips.erase(comp->Clips.begin() + index);
		return old_size != comp->Clips.size();
	}

	void AudioSystem::AudioSourceAddFilter(AudioSourceComponent* comp, AudioFilterFlags filter)
	{
		comp->Filter.Flags |= filter;
	}

	void AudioSystem::AudioSourceRemoveFilter(AudioSourceComponent* comp, AudioFilterFlags filter)
	{
		comp->Filter.Flags &= ~filter;
	}

	void AudioSystem::AudioSourceStopAll(AudioSourceComponent* comp)
	{
		SoLoud::Bus* mixer = comp->Mixer->_Cast<SoLoud::Bus>();
		if (mixer->getActiveVoiceCount() > 0)
		{
			m_State->Core->stop(*comp->GroupHandle);
		}

		ClearFilters(comp);
	}

	void AudioSystem::AudioSourcePlayAll(AudioSourceComponent* comp)
	{
		for (auto& clip : comp->Clips)
		{
			AudioSystem::CreateFilters(comp);
			AudioSystem::PlayClip(comp, clip.m_Clip, clip.m_Handle);
		}
	}

	float* AudioSystem::GetFFT()
	{
		return m_State->Core->calcFFT();
	}

	float* AudioSystem::GetWave()
	{
		return m_State->Core->getWave();
	}

	void AudioSystem::SetAudioSourceVolume(AudioSource* source, float value)
	{
		m_State->Core->setVolume(*source->GroupHandle, value);
	}

	void AudioSystem::SetAudioSourceSpeed(AudioSource* source, float value)
	{
		m_State->Core->setRelativePlaySpeed(*source->GroupHandle, value * 10.0f);
	}

	void AudioSystem::SetGlobalVolume(float value)
	{
		m_State->Core->setGlobalVolume(value);
	}

	void AudioSystem::StopAll()
	{
		m_State->Core->stopAll();
	}

	void AudioSystem::RemoveHandle(uint32_t handle)
	{
		m_State->Handles.erase(std::remove_if(m_State->Handles.begin(), m_State->Handles.end(),
			[&](const Ref<AudioHandle>& another) { return *another == handle; }), m_State->Handles.end());
	}

	void AudioSystem::CreateFilters(AudioSourceComponent* comp)
	{
		SoLoud::Bus* mixer = comp->Mixer->_Cast<SoLoud::Bus>();
		AudioSourceFilter& ASfilter = comp->Filter;

		if ((ASfilter.Flags & AudioFilterFlags::Echo) == AudioFilterFlags::Echo)
		{
			SoLoud::EchoFilter filter;
			filter.setParams(ASfilter.EchoCI.Delay, ASfilter.EchoCI.Decay, ASfilter.EchoCI.Filter);
			mixer->setFilter(comp->FilterCount, &filter);
			comp->FilterCount++;
		}

		if ((ASfilter.Flags & AudioFilterFlags::Bassboost) == AudioFilterFlags::Bassboost)
		{
			SoLoud::BassboostFilter filter;
			filter.setParams(ASfilter.BassboostCI.Boost);
			mixer->setFilter(comp->FilterCount, &filter);
			comp->FilterCount++;
		}

		if ((ASfilter.Flags & AudioFilterFlags::BQResonant) == AudioFilterFlags::BQResonant)
		{
			SoLoud::BiquadResonantFilter filter;
			filter.setParams(ASfilter.BQResonantCI.Type, ASfilter.BQResonantCI.Frequency, ASfilter.BQResonantCI.Resonance);
			mixer->setFilter(comp->FilterCount, &filter);
			comp->FilterCount++;
		}

		if ((ASfilter.Flags & AudioFilterFlags::Freeverb) == AudioFilterFlags::Freeverb)
		{
			SoLoud::FreeverbFilter filter;
			filter.setParams(ASfilter.FreeverbCI.Mode, ASfilter.FreeverbCI.RoomSize, ASfilter.FreeverbCI.Damp, ASfilter.FreeverbCI.Width);
			mixer->setFilter(comp->FilterCount, &filter);
			comp->FilterCount++;
		}

		if ((ASfilter.Flags & AudioFilterFlags::Lofi) == AudioFilterFlags::Lofi)
		{
			SoLoud::LofiFilter filter;
			filter.setParams(ASfilter.LofiCI.SampleRate, ASfilter.LofiCI.Bitdepth);
			mixer->setFilter(comp->FilterCount, &filter);
			comp->FilterCount++;
		}

		if ((ASfilter.Flags & AudioFilterFlags::WaveShaper) == AudioFilterFlags::WaveShaper)
		{
			SoLoud::WaveShaperFilter filter;
			filter.setParams(ASfilter.WaveShaperCI.Amount);
			mixer->setFilter(comp->FilterCount, &filter);
			comp->FilterCount++;
		}
	}

	void AudioSystem::ClearFilters(AudioSourceComponent* comp)
	{
		SoLoud::Bus* mixer = comp->Mixer->_Cast<SoLoud::Bus>();
		for (uint32_t i = 0; i < comp->FilterCount; ++i)
		{
			mixer->setFilter(i, NULL);
		}

		comp->FilterCount = 0;
	}

	void AudioSystem::OnBeginWorld()
	{
		entt::registry* registry = m_World->m_CurrentRegistry;
		const auto& view = registry->view<AudioSourceComponent>();
		for (auto entity : view)
		{
			AudioSourceComponent& component = view.get<AudioSourceComponent>(entity);
			AudioSystem::ClearFilters(&component);
			AudioSystem::CreateFilters(&component);

			for (auto& info : component.Clips)
			{
				if (info.m_CreateInfo.bPlayOnAwake)
				{
					AudioSystem::PlayClip(&component, info.m_Clip, info.m_Handle);
				}
			}
		}
	}

	void AudioSystem::OnEndWorld()
	{
		entt::registry* registry = m_World->m_CurrentRegistry;
		const auto& view = registry->view<AudioSourceComponent>();
		for (auto entity : view)
		{
			AudioSourceComponent& component = view.get<AudioSourceComponent>(entity);
			AudioSystem::AudioSourceStopAll(&component);
		}
	}

	void AudioSystem::OnUpdate(const glm::vec3 camPos)
	{
		m_State->Core->set3dListenerPosition(camPos.x, camPos.y, camPos.z);
		{
			entt::registry* registry = m_World->m_CurrentRegistry;
			const auto& view = registry->view<AudioSourceComponent, TransformComponent>();

			for (auto entity : view)
			{
				auto& [as, transform] = view.get<AudioSourceComponent, TransformComponent>(entity);
				for (auto& clip_info : as.Clips)
				{
					AudioClipCreateInfo& create_info = clip_info.m_CreateInfo;
					if (!create_info.bStatic && create_info.eType == ClipType::Sound_3D)
					{
						m_State->Core->set3dSourceParameters(*as.GroupHandle, transform.WorldPos.x, transform.WorldPos.y, transform.WorldPos.z, transform.Rotation.x, transform.Rotation.y, transform.Rotation.z);
					}
				}
			}
		}
		m_State->Core->update3dAudio();
	}
}