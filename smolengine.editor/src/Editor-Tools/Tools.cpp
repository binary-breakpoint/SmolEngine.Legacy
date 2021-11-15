#include "stdafx.h"
#include "Editor-Tools/Tools.h"
#include "ImGuiExtension.h"

namespace SmolEngine
{
	bool Tools::FileExtensionCheck(std::string& path, const std::string& ext)
	{
		std::filesystem::path p(path);
		if (p.extension().filename() == ext)
		{
			return true;
		}

		return false;
	}

	bool Tools::FileBrowserDragAndDropCheck(const std::string& ext, std::string& out_path)
	{
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FileBrowser"))
			{
				std::string& path = *(std::string*)payload->Data;
				if (FileExtensionCheck(path, ext))
				{
					out_path = path;
					return true;
				}

			}
			ImGui::EndDragDropTarget();
		}

		return false;
	}
}