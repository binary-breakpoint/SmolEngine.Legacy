#pragma once
#include "Core/Core.h"

#include <string>
#include <glm/glm.hpp>

namespace cereal
{
	class access;
}

namespace SoLoud
{
	class Wav;
};

namespace SmolEngine
{
	enum class ClipType: uint16_t
	{
		Sound_2D,
		Sound_3D,
		Sound_Background
	};

	enum class ClipSampleRate: uint16_t
	{
		HZ_8000,
		HZ_22050,
		HZ_4410,
		HZ_4800
	};

	struct AudioClipCreateInfo
	{
		bool             bStatic = false;
		bool             bLoop = false;
		bool             bPlayOnAwake = false;
		ClipType         eType = ClipType::Sound_2D;
		ClipSampleRate   eSampleRate = ClipSampleRate::HZ_4410;
		float            Volume = 1.0f;
		float            Speed = 1.0f;
		float            LoopTime = 0.0f;
		float            DopplerFactor = 1.0f;
		glm::vec2        MinMaxDistance = glm::vec2(0, 1000);
		glm::vec3        WorldPos = glm::vec3(0);
		glm::vec3        Velocity = glm::vec3(0);
		std::string      FilePath = "";
		std::string      FileName = "";

		bool Load(const std::string& filePath);
		bool Save(const std::string& filePath);

	private:
		friend class cereal::access;

		template<typename Archive>
		void serialize(Archive& archive)
		{
			archive(bStatic, bLoop, bPlayOnAwake, eType, eSampleRate, Volume, Speed, LoopTime, DopplerFactor, MinMaxDistance.x, MinMaxDistance.y,
				WorldPos.x, WorldPos.y, WorldPos.z, Velocity.x, Velocity.y, Velocity.z, FilePath, FileName);
		}
	};

	struct AudioClip
	{
		AudioClip() = default;
		AudioClip(AudioClipCreateInfo& createInfo);

		void                         Clear();
		void                         UpdateInfo(AudioClipCreateInfo& createInfo);
		bool                         IsValid() const;
		float                        GetLength();
		const AudioClipCreateInfo&   GetCretaeInfo() const;

	private:
		SoLoud::Wav*          m_Obj = nullptr;
		AudioClipCreateInfo   m_CreateInfo{};

		friend class AudioSystem;
		friend struct AudioSourceComponent;
	};
}