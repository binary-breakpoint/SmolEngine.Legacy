#include "stdafx.h"
#include "ECS/Systems/AudioSystem.h"

#include "ECS/Components/Include/Components.h"
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
	float* AudioSystem::GetFFT()
	{
		return m_State->Core->calcFFT();
	}

	float* AudioSystem::GetWave()
	{
		return m_State->Core->getWave();
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

	void AudioSystem::OnBeginWorld()
	{
		entt::registry* registry = m_World->m_CurrentRegistry;
		const auto& view = registry->view<AudioSourceComponent>();
		for (auto entity : view)
		{
			AudioSourceComponent& component = view.get<AudioSourceComponent>(entity);
			component.ClearFiltersEX();
			component.CreateFiltersEX();

			for (auto& info : component.Clips)
			{
				if (info.m_CreateInfo.bPlayOnAwake)
				{
					component.PlayClip(info.m_Clip, info.m_Handle);
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
			component.StopAll();
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
						m_State->Core->set3dSourceParameters(*as.GroupHandle, transform.WorldPos.x, transform.WorldPos.y, 
							transform.WorldPos.z, transform.Rotation.x, transform.Rotation.y, transform.Rotation.z);
					}
				}
			}
		}
		m_State->Core->update3dAudio();
	}
}