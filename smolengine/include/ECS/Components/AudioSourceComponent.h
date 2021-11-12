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
	struct AudioSourceComponent: public BaseComponent, public AudioSource
	{
		AudioSourceComponent() = default;
		AudioSourceComponent(uint32_t id)
			: BaseComponent(id) {}

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

		std::vector<ClipInstance> Clips;

	public:
		ClipInstance* GetClipByIndex(uint32_t index) { return &Clips[index]; }

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