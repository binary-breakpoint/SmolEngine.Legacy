#pragma once

#include "Core/Core.h"
#include "TexturesLoader.h"

#include <vector>
#include <ctime>
#include <sstream>

#ifndef FROSTIUM_SMOLENGINE_IMPL
#define FROSTIUM_SMOLENGINE_IMPL
#endif
#include <frostium/Common/DebugLog.h>
#include <imgui/imgui.h>

namespace SmolEngine
{
	struct Message
	{
		ImVec4        Color = ImVec4{ 1.0f, 1.0f, 1.0f, 1.0f };
		std::string   Text = std::string("");
		LogLevel      Type = LogLevel::Info;
	};

	class ConsolePanel
	{
	public:

		ConsolePanel() { s_Instance = this; }
		~ConsolePanel() { s_Instance = nullptr; }

		void Update(bool& enabled)
		{
			if (enabled)
			{
				if (ImGui::Begin("Console", &enabled))
				{
					ImGui::BeginChild("ConsoleChild");
					{
						ImGui::SetWindowFontScale(0.9f);

						ImGui::SameLine();
						ImGui::Combo("Filter", &m_CurrentItem, "None\0Info\0Error\0Warn\0\0");
						ImGui::SameLine();
						if (ImGui::Button("Clear")) { m_Messages.clear(); }

						ImGui::Separator();
						ImGui::BeginChild("Log");
						{
							for (auto& msg : m_Messages)
							{
								if (m_CurrentItem == 0 ? true: m_CurrentItem == (int)msg.Type)
								{
									ImGui::Image(GetImageDescriptor(msg.Type), m_ButtonSize);

									ImGui::SameLine();
									ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6);
									ImGui::PushStyleColor(ImGuiCol_Text, msg.Color);
									ImGui::TextWrapped(msg.Text.c_str());
									ImGui::PopStyleColor();
								}
							}
						}
						ImGui::EndChild();
					}
					ImGui::EndChild();
				}

				ImGui::End();
			}
			
		}

		void* GetImageDescriptor(LogLevel level)
		{
			switch (level)
			{
			case LogLevel::Info: return TexturesLoader::Get()->m_InfoIcon.GetImGuiTexture();
			case LogLevel::Warning: return TexturesLoader::Get()->m_WarningIcon.GetImGuiTexture();
			case LogLevel::Error: return TexturesLoader::Get()->m_ErrorIcon.GetImGuiTexture();
			}

			return nullptr;
		}

		void AddMessage(const std::string& message, LogLevel logLevel)
		{
			Message msg;
			msg.Type = logLevel;

			time_t theTime = time(NULL);
			struct tm* aTime = localtime(&theTime);
			int hour = aTime->tm_hour;
			int min = aTime->tm_min;
			int second = aTime->tm_sec;
			std::ostringstream oss;

			switch (logLevel)
			{
			case LogLevel::Info: msg.Color = ImVec4{ 1.0f, 1.0f, 1.0f, 1.0f }; break;
			case LogLevel::Warning: msg.Color = ImVec4{ 2.2f, 1.2f, 0.1f, 1.0f }; break;
			case LogLevel::Error: msg.Color = ImVec4{ 0.99f, 0.1f, 0.1f, 1.0f }; break;
			}

			bool ext_found = message.find("[") != std::string::npos && message.find("]") != std::string::npos;
			if (ext_found == false) { oss << "[Core]: "; }
			oss << message;
			oss << " - " << hour << ":" << min << ":" << second;

			msg.Text = oss.str();
			m_Messages.push_back(msg);
		}

		void AddMessageInfo(const std::string& message)
		{
			AddMessage(message, LogLevel::Info);
		}

		void AddMessageWarn(const std::string& message)
		{
			AddMessage(message, LogLevel::Warning);
		}

		void AddMessageError(const std::string& message)
		{
			AddMessage(message, LogLevel::Error);
		}

		static ConsolePanel* GetConsole() { return s_Instance; }

	private:

		inline static ConsolePanel*  s_Instance = nullptr;
		int                           m_CurrentItem = 0;
		ImVec2                        m_ButtonSize = ImVec2(25, 25);
		std::vector<Message>          m_Messages;
	};
}