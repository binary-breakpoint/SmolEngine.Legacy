#include "stdafx.h"
#include "Audio/AudioClip.h"

#include <soloud_wav.h>
#include <cereal/archives/json.hpp>

namespace SmolEngine
{
	AudioClip::AudioClip(AudioClipCreateInfo& createInfo)
	{
		if (std::filesystem::exists(createInfo.FilePath) && !m_Obj)
		{
			std::filesystem::path p(createInfo.FilePath);
			createInfo.FileName = p.filename().u8string();

			m_CreateInfo = createInfo;
			m_Obj = new SoLoud::Wav();
			m_Obj->load(m_CreateInfo.FilePath.c_str());
			if (m_CreateInfo.eType == ClipType::Sound_3D)
			{
				m_Obj->set3dMinMaxDistance(m_CreateInfo.MinMaxDistance.x, m_CreateInfo.MinMaxDistance.y);
				m_Obj->set3dAttenuation(SoLoud::AudioSource::EXPONENTIAL_DISTANCE, 0.5);
			}
		}
	}

	void AudioClip::Clear()
	{
		if (m_Obj)
		{
			delete m_Obj;
			m_Obj = nullptr;
		}
	}

	bool AudioClip::IsValid() const
	{
		if(m_Obj)
		{
			return m_Obj->getLength() > 0.0f;
		}

		return false;
	}

	void AudioClip::UpdateInfo(AudioClipCreateInfo& info)
	{
		std::filesystem::path p(info.FilePath);
		info.FileName = p.filename().u8string();

		m_CreateInfo = info;
	}

	float AudioClip::GetLength()
	{
		if (m_Obj)
		{
			return static_cast<float>(m_Obj->getLength());
		}

		return 0.0f;
	}

	const AudioClipCreateInfo& AudioClip::GetCretaeInfo() const
	{
		return m_CreateInfo;
	}

	bool AudioClipCreateInfo::Load(const std::string& filePath)
	{
		std::stringstream storage;
		std::ifstream file(filePath);
		if (!file)
		{
			DebugLog::LogError("Could not open the file: {}", filePath);
			return false;
		}

		storage << file.rdbuf();
		{
			cereal::JSONInputArchive input{ storage };
			input(bStatic, bLoop, bPlayOnAwake, eType, eSampleRate, Volume, Speed, LoopTime, DopplerFactor, MinMaxDistance.x, MinMaxDistance.y, WorldPos.x, WorldPos.y, WorldPos.z, Velocity.x, Velocity.y, Velocity.z, FilePath, FileName);
		}

		return true;
	}

	bool AudioClipCreateInfo::Save(const std::string& filePath)
	{
		std::stringstream storage;
		{
			cereal::JSONOutputArchive output{ storage };
			serialize(output);
		}

		std::ofstream myfile(filePath);
		if (myfile.is_open())
		{
			myfile << storage.str();
			myfile.close();
			return true;
		}

		return false;
	}
}