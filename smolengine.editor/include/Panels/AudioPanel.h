#pragma once

#include "Audio/AudioClip.h"
#include "Audio/AudioHandle.h"
#include "Audio/AudioSource.h"

namespace SmolEngine
{
	class AudioPanel
	{
	public:
		void               Update();
		void               Reset();
		void               Open(const std::string& filePath);

	private:
		Ref<AudioHandle>       m_Handle = nullptr;
		Ref<AudioClip>        m_Clip = nullptr;
		Scope<AudioSource>    m_Source = nullptr;
		std::string           m_Path = "";
		int                   m_TypeFlag = 0;
		int                   m_SampleRateFlag = 0;
		AudioClipCreateInfo   m_CreateInfo{};
	};
}