#include "stdafx.h"
#include "Panels/AudioPanel.h"
#include "ECS/Systems/AudioSystem.h"
#include "ImGuiExtension.h"

namespace SmolEngine
{
	void AudioPanel::Update()
	{
		if (!m_Path.empty())
		{
			float* fft = AudioSystem::GetFFT();
			ImGui::PlotHistogram("##Mixer", fft, 256 / 2, 0, "FFT", 0, 10, ImVec2(ImGui::GetContentRegionAvail().x, 80), 8);

			float* wave = AudioSystem::GetWave();
			ImGui::PlotLines("##Wave", wave, 256, 0, "Wave", -1, 1, ImVec2(ImGui::GetContentRegionAvail().x, 80));
			ImGui::NewLine();

			bool update = false;
			if (ImGui::Extensions::Combo("Type", "Sound 2D\0Sound 3D\0Background\0", m_TypeFlag))
			{
				m_CreateInfo.eType = (ClipType)m_TypeFlag;
				update = true;
			}

			if (ImGui::Extensions::Combo("Sample Rate", "Rate 8000hz\0Rate 22050hz\0Rate 4410hz\0Rate 4800hz\0", m_SampleRateFlag))
			{
				m_CreateInfo.eSampleRate = (ClipSampleRate)m_SampleRateFlag;
				update = true;
			}

			if(ImGui::Extensions::DragFloat("Volume", m_CreateInfo.Volume, 0.5f, 0.0f, 100.0f))
				update = true;

			if (m_CreateInfo.eType == ClipType::Sound_3D)
			{
				if (ImGui::Extensions::Slider("DopplerFactor", m_CreateInfo.DopplerFactor, 0.0f, 100.0f))
					update = true;
			}

			if (ImGui::Extensions::DragFloat("Speed", m_CreateInfo.Speed, 1.0f, 0.0f, 10))
				update = true;

			if(ImGui::Extensions::Slider("Loop Point", m_CreateInfo.LoopTime, 0.0f, m_Clip->GetLength()))
				update = true;

			if (m_CreateInfo.eType == ClipType::Sound_3D)
			{
				if (ImGui::Extensions::InputFloat("Min Disatnce", m_CreateInfo.MinMaxDistance.x))
					update = true;

				if (ImGui::Extensions::InputFloat("Max Disatnce", m_CreateInfo.MinMaxDistance.y))
					update = true;

				if (m_CreateInfo.bStatic)
				{
					if (ImGui::Extensions::InputFloat3("Position", m_CreateInfo.WorldPos))
						update = true;
				}
			}

			if (ImGui::Extensions::CheckBox("Loop", m_CreateInfo.bLoop))
				update = true;

			if (ImGui::Extensions::CheckBox("PlayOnAwake ", m_CreateInfo.bPlayOnAwake))
				update = true;

			if (m_CreateInfo.eType == ClipType::Sound_3D)
			{
				if (ImGui::Extensions::CheckBox("Static", m_CreateInfo.bStatic))
					update = true;
			}

			if (update)
			{
				m_CreateInfo.Save(m_Path);
			}

			ImGui::NewLine();
			ImGui::SetCursorPosX(6.0f);
			if (ImGui::Button("Play"))
			{
				if (m_Handle)
					m_AS.StopClip(m_Handle);

				m_Clip->UpdateInfo(m_CreateInfo);
				m_AS.PlayClip(m_Clip, m_Handle);
			}
			ImGui::SameLine();
			if (ImGui::Button("Stop"))
			{
				if (m_Handle)
					m_AS.StopClip(m_Handle);
			}
		}
	}

	void AudioPanel::Reset()
	{
		if (m_Handle)
		{
			m_AS.StopClip(m_Handle);
			m_Handle.reset();
		}

		m_Clip.reset();
		m_Path = "";
	}

	void AudioPanel::Open(const std::string& filePath)
	{
		if (m_CreateInfo.Load(filePath))
		{
			m_Clip = std::make_shared<AudioClip>(m_CreateInfo);
			m_Path = filePath;
			m_TypeFlag = (int)m_CreateInfo.eType;
			m_SampleRateFlag = (int)m_CreateInfo.eSampleRate;
		}
	}
}