#pragma once
#include "ECS/Components/BaseComponent.h"

#include "Audio/AudioClip.h"
#include "Audio/AudioHandle.h"
#include "Audio/AudioSource.h"

namespace cereal
{
	class access;
}

namespace SmolEngine
{
	struct AudioEngineSComponent;

	struct ClipInstance
	{
		Ref<AudioHandle>      m_Handle = nullptr;
		Ref<AudioClip>        m_Clip = nullptr;
		AudioClipCreateInfo   m_CreateInfo{};

	private:
		friend class cereal::access;

		template<typename Archive>
		void serialize(Archive& archive)
		{
			archive(m_CreateInfo);
		}
	};

	struct AudioSourceComponent: public BaseComponent, public AudioSource
	{
		AudioSourceComponent();
		AudioSourceComponent(uint32_t id);

		bool          PlayClip(Ref<AudioClip>& sound, Ref<AudioHandle>& out_handle, bool add_to_mixer = true);
		bool          StopClip(const Ref<AudioHandle>& handle);
		bool          PauseClip(const Ref<AudioHandle>& handle, bool pause);
		bool          SetLooping(const Ref<AudioHandle>& handle, bool value, float timePoint);
		bool          SetSpeed(const Ref<AudioHandle>& handle, float value);
		void          SetVolume(float value);
		void          SetSpeed(float value);
		bool          SetSampleRate(const Ref<AudioHandle>& handle, ClipSampleRate rate);
		bool          SetPosition(const Ref<AudioHandle>& handle, const glm::vec3& postion);
		void          LoadClip(const std::string& path);
		void          LoadClip(AudioClipCreateInfo* info);
		void          AddFilter(AudioFilterFlags filter);
		void          RemoveFilter(AudioFilterFlags filter);
		bool          RemoveClipAtIndex(uint32_t index);
		ClipInstance* GetClipByIndex(uint32_t index);
		void          CreateFiltersEX();
		void          ClearFiltersEX();
		void          StopAll();
		void          PlayAll();

	public:
		std::vector<ClipInstance> Clips;
		AudioEngineSComponent*    EnginePtr = nullptr;

	private:
		friend class cereal::access;
		friend class EditorLayer;
		friend class WorldAdmin;

		template<typename Archive>
		void serialize(Archive& archive)
		{
			archive(Clips, Volume, Speed, Filter);
		}
	};
}