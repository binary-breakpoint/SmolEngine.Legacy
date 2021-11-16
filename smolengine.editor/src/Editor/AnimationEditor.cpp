#include "stdafx.h"
#include "Editor/AnimationEditor.h"
#include "ImGuiExtension.h"
#include "Editor-Tools/Tools.h"
#include "ECS/Components/MeshComponent.h"

namespace SmolEngine
{
	AnimationEditor::AnimationEditor()
	{
		m_Context = ed::CreateEditor();
	}

	void AnimationEditor::Update(bool& is_open, MeshComponent* comp)
	{
		if (!is_open)
			return;

		if (ImGui::Begin("Animation Controller", &is_open))
		{
			DrawToolBar(comp);
			DrawNodeEditor(comp);
			DrawPopUp(comp);
		}
		ImGui::End();
	}

	void AnimationEditor::Reset()
	{
		m_SelectedNode = "";
		m_Buffer = "";
	}

	void AnimationEditor::DrawToolBar(MeshComponent* comp)
	{
		ImGui::PushID("AnimationEditorToolBarWindowAddID");
		ImGui::BeginChild("AnimationEditorToolBarWindow", { ImGui::GetContentRegionAvail().x / 3.0f,  ImGui::GetContentRegionAvail().y}, true);
		{
			if (!m_SelectedNode.empty())
			{
				auto& node = comp->GetAnimationController()->GetClip(m_SelectedNode);
				float time = node->GetTimeRatio();

				ImGui::Extensions::SetItemPos(70.0f);
				if (ImGui::Extensions::DragFloat("Time", time, 0.1f, 0.0f, node->GetDuration()))
					node->SetTimeRatio(time);

				ImGui::Extensions::DragFloat("Speed", node->GetProperties().Speed);
				ImGui::Extensions::CheckBox("Play", node->GetProperties().bPlay);
				ImGui::Extensions::CheckBox("Loop", node->GetProperties().bLoop);
				ImGui::Extensions::RestorePos();
			}
		}
		ImGui::EndChild();
		ImGui::PopID();

	}

	void AnimationEditor::DrawNodeEditor(MeshComponent* comp)
	{
		ImGui::SameLine();
		ImGui::BeginChild("AnimationEditorWindow");
		{
			ed::SetCurrentEditor(m_Context);
			ed::Begin("AnimationEditorEX", ImVec2(0.0, 0.0f));
			{
				std::string path = "";
				if (Tools::FileBrowserDragAndDropCheck(".s_animation", path))
					comp->LoadAnimation(path);
			}

			{
				int uniqueId = 1;
				const auto& clips = comp->GetAnimationController()->GetClips();
				for (auto& [name, clip] : clips)
				{
					ed::BeginNode(uniqueId);
					if (ed::GetDoubleClickedNode() == (ed::NodeId)uniqueId)
					{
						m_bOpenPopUp = true;
						m_Buffer = name;
					}

					if (m_SelectedNode == name) { ed::SelectNode(uniqueId); }
					ImGui::Text(name.c_str());

					// Pins
					{
						ed::BeginPin(uniqueId++, ed::PinKind::Input);
						ImGui::Text("-> In");
						ed::EndPin();
						ImGui::SameLine();
						ed::BeginPin(uniqueId++, ed::PinKind::Output);
						ImGui::Text("Out ->");
						ed::EndPin();
					}
					ed::EndNode();
					uniqueId++;
				}
			}
			ed::End();
			ed::SetCurrentEditor(nullptr);

		}

		ImGui::EndChild();
	}

	void AnimationEditor::DrawPopUp(MeshComponent* comp)
	{
		CheckAndOpenPopUp();
		if (ImGui::BeginPopup("AnimationEditor_PopUp_Actions"))
		{
			if (ImGui::MenuItem("set active"))
			{
				auto& clip = comp->GetAnimationController()->GetClip(m_Buffer);
				if (clip)
				{
					comp->GetAnimationController()->SetActiveClip(m_Buffer);
					m_SelectedNode = m_Buffer;
				}

				ClosePopUp();
			}

			if (ImGui::MenuItem("play"))
			{
				auto& clip = comp->GetAnimationController()->GetClip(m_Buffer);
				if (clip)
				{
					comp->GetAnimationController()->SetActiveClip(m_Buffer);
					m_SelectedNode = m_Buffer;
					clip->GetProperties().bPlay = true;
				}

				ClosePopUp();
			}

			if (ImGui::MenuItem("stop"))
			{
				auto& clip = comp->GetAnimationController()->GetClip(m_Buffer);
				if (clip)
				{
					comp->GetAnimationController()->SetActiveClip(m_Buffer);
					m_SelectedNode = m_Buffer;
					clip->GetProperties().bPlay = false;
				}

				ClosePopUp();
			}

			if (ImGui::MenuItem("remove"))
			{
				comp->GetAnimationController()->RemoveClip(m_Buffer);
				m_SelectedNode = "";

				ClosePopUp();
			}

			ImGui::EndPopup();
		}
	}

	void AnimationEditor::ClosePopUp()
	{
		m_Buffer = "";
		ImGui::CloseCurrentPopup();
	}

	void AnimationEditor::CheckAndOpenPopUp()
	{
		if (m_bOpenPopUp)
		{
			ImGui::OpenPopup("AnimationEditor_PopUp_Actions", ImGuiPopupFlags_NoOpenOverExistingPopup);
			m_bOpenPopUp = false;
		}
	}
}