#include "stdafx.h"
#include "Editor-Tools/OzzTool.h"
#include "ImGuiExtension.h"
#ifndef FROSTIUM_SMOLENGINE_IMPL
#define FROSTIUM_SMOLENGINE_IMPL
#endif
#include <frostium/Import/OzzImported.h>

namespace SmolEngine
{
	void OzzTool::Update(bool& open)
	{
		if (!open)
			return;

		if (ImGui::Begin("Ozz Converter", &open))
		{
			ImGui::Extensions::SetItemPos(70.0f);
			ImGui::Extensions::InputRawString("Gltf", m_Path, "", ImGuiInputTextFlags_ReadOnly);
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FileBrowser"))
				{
					std::string& path = *(std::string*)payload->Data;
					std::filesystem::path p(path);
					if (p.extension().filename() == ".gltf")
						m_Path = path;

				}
				ImGui::EndDragDropTarget();
			}

			if (ImGui::Button("Convert", { ImGui::GetWindowWidth() - 20.0f, 30.0f }))
			{
				if (!m_Path.empty())
					OzzImporter::ImportGltf(m_Path);
			}

			ImGui::Extensions::RestorePos();
		}

		ImGui::End();
	}
}