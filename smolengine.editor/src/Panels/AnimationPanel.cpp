#include "stdafx.h"
#include "Panels/AnimationPanel.h"
#include "Editor-Tools/Tools.h"
#include "ImGuiExtension.h"

#include <imgui/misc/cpp/imgui_stdlib.h>

namespace SmolEngine
{
	void AnimationPanel::Update()
	{
		if (m_CurrentPath.empty())
			return;

		ImGui::PushID("AnimationHandlerAddID");
		ImGui::InputTextWithHint("Search", "", &m_SearchBuffer);
		if (ImGui::BeginListBox("##OzzConverter_Listbox", ImVec2(-FLT_MIN, 5 * ImGui::GetTextLineHeightWithSpacing())))
		{
			for (uint32_t i = 0; i < static_cast<uint32_t>(m_Files.size()); i ++)
			{
				const std::string& path = m_Files[i];
				if (path.find(m_SearchBuffer) == std::string::npos)
					continue;

				const bool is_selected = (m_ItemIndex == i);
				{
					std::filesystem::path p(path);
					const bool click = ImGui::Selectable(p.filename().u8string().c_str(), is_selected);
					if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
					{
						ImGui::SetDragDropPayload("AnimationPanel", &m_Files[i], sizeof(std::string));
						ImGui::Text(p.filename().u8string().c_str());
						ImGui::EndDragDropSource();
					}

					if (click)
						m_ItemIndex = i;
				}

				// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndListBox();
		}

		ImGui::NewLine();

		ImGui::Extensions::InputRawString("Skeleton", m_CreateInfo.SkeletonPath, "", ImGuiInputTextFlags_ReadOnly);
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("AnimationPanel"))
				m_CreateInfo.SkeletonPath = *(std::string*)payload->Data;

			ImGui::EndDragDropTarget();
		}

		ImGui::Extensions::InputRawString("Animation", m_CreateInfo.AnimationPath, "", ImGuiInputTextFlags_ReadOnly);
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("AnimationPanel"))
				m_CreateInfo.AnimationPath = *(std::string*)payload->Data;

			ImGui::EndDragDropTarget();
		}

		ImGui::Extensions::InputRawString("Model", m_CreateInfo.ModelPath, "", ImGuiInputTextFlags_ReadOnly);
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("AnimationPanel"))
				m_CreateInfo.ModelPath = *(std::string*)payload->Data;

			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FileBrowser"))
				Tools::FileExtensionCheck(m_CreateInfo.ModelPath, ".gltf");

			ImGui::EndDragDropTarget();
		}

		ImGui::NewLine();
		ImGui::Separator();
		ImGui::NewLine();

		ImGui::Extensions::InputRawString("Clip Name", m_CreateInfo.Name);
		ImGui::Extensions::InputFloat("Speed", m_CreateInfo.ClipInfo.Speed);
		ImGui::Extensions::CheckBox("Play", m_CreateInfo.ClipInfo.bPlay);
		ImGui::Extensions::CheckBox("Loop", m_CreateInfo.ClipInfo.bLoop);

		ImGui::NewLine();
		if (ImGui::Button("Save", { ImGui::GetWindowWidth() - 20.0f, 30.0f }))
		{
			if (!m_CreateInfo.ModelPath.empty() &&
				!m_CreateInfo.AnimationPath.empty() &&
				!m_CreateInfo.SkeletonPath.empty() &&
				!m_CreateInfo.Name.empty())
			{
				m_CreateInfo.Save(m_CurrentPath);
			}
		}

		ImGui::PopID();
	}

	void AnimationPanel::Reset()
	{
		m_CreateInfo = {};
		m_CurrentPath = "";
		m_SearchBuffer = "";
		m_ItemIndex = 0;
		m_Files.clear();
	}

	void AnimationPanel::Open(const std::string& filePath, bool is_new)
	{
		if (!is_new)
		{
			m_CreateInfo.Load(filePath);
		}

		m_CurrentPath = filePath;
		m_Files.clear();

		for (const auto& dir : std::filesystem::recursive_directory_iterator(Engine::GetEngine()->GetAssetsFolder()))
		{
			const auto& path = dir.path();
			if (path.extension() == ".ozz" || path.extension() == ".gltf")
				m_Files.emplace_back(path.u8string());
		}
	}
}