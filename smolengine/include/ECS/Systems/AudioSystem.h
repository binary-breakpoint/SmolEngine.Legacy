#pragma once
#include "Core/Core.h"
#include "Audio/AudioClip.h"
#include "Audio/AudioHandle.h"

#include <glm/glm.hpp>
#include <string>

namespace SmolEngine
{
	enum class AudioFilterFlags : int;

	struct WorldAdminStateSComponent;
	struct AudioSourceComponent;
	struct AudioEngineSComponent;
	struct AudioSource;

	class AudioSystem
	{
	public:
		static bool      PlayClip(AudioSource* source, Ref<AudioClip>& sound, Ref<AudioHandle>& out_handle, bool add_to_mixer = true);
		static bool      StopClip(AudioSource* source, const Ref<AudioHandle>& handle);
		static bool      PauseClip(AudioSource* source, const Ref<AudioHandle>& handle, bool pause);
		static bool      RemoveClipAtIndex(AudioSourceComponent* comp, uint32_t index);
		static bool      SetLooping(AudioSource* source, const Ref<AudioHandle>& handle, bool value, float timePoint);
		static bool      SetSpeed(AudioSource* source, const Ref<AudioHandle>& handle, float value);
		static bool      SetSampleRate(AudioSource* source, const Ref<AudioHandle>& handle, ClipSampleRate rate);
		static bool      Set3DPosition(AudioSource* source, const Ref<AudioHandle>& handle, const glm::vec3& postion);
		static void      AudioSourceAddFilter(AudioSourceComponent* comp, AudioFilterFlags filter);
		static void      AudioSourceRemoveFilter(AudioSourceComponent* comp, AudioFilterFlags filter);
		static void      AudioSourceStopAll(AudioSourceComponent* comp);
		static void      AudioSourcePlayAll(AudioSourceComponent* comp);
		static void      SetAudioSourceVolume(AudioSource* source, float value);
		static void      SetAudioSourceSpeed(AudioSource* source, float value);
		static void      SetGlobalVolume(float value);
		static void      StopAll();

		static float*    GetFFT();
		static float*    GetWave();

	private:
		static void      RemoveHandle(uint32_t handle);
		static void      CreateFilters(AudioSourceComponent* comp);
		static void      ClearFilters(AudioSourceComponent* comp);

		static void      OnBeginWorld();
		static void      OnEndWorld();
		static void      OnUpdate(const glm::vec3 camPos);

	private:
		inline static AudioEngineSComponent*     m_State = nullptr;
		inline static WorldAdminStateSComponent* m_World = nullptr;

		friend class EditorLayer;
		friend class GameView;
		friend class WorldAdmin;
	};
}