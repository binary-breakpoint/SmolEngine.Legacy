#include "stdafx.h"
#include "Panels/FileExplorer.h"
#include "Panels/MaterialPanel.h"
#include "Multithreading/JobsSystem.h"
#include "Materials/PBRFactory.h"
#include "Primitives/Texture.h"
#include "Primitives/Framebuffer.h"
#include "Audio/AudioClip.h"
#include "ECS/Prefab.h"

#include <imgui/misc/cpp/imgui_stdlib.h>

namespace SmolEngine
{
	FileExplorer::FileExplorer()
		:
		m_ButtonSize(64.0f, 64.0f),
		m_SelectColor(0.984f, 0.952f, 0.356f, 1.0f),
		m_SearchBuffer("")
	{

	}

	void FileExplorer::Create(const std::string& current_path)
	{
		m_pTextureLoader = TexturesLoader::Get();
		std::filesystem::path p(current_path);
		m_CurrentDir = p.u8string();
		m_HomeDir = m_CurrentDir;
		Import();
	}

	void FileExplorer::ClearSelection()
	{
		m_SelectedNode = "";
		ClosePopUp();
	}

	void FileExplorer::Update()
	{
		ImGui::Begin("File Explorer", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);
		{
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ActorDragAndDrop"))
				{
					uint32_t id = *static_cast<uint32_t*>(payload->Data);
					Ref<Actor> target = WorldAdmin::GetSingleton()->GetActiveScene()->FindActorByID(id);

					Prefab pref;
					std::string savePath = m_CurrentDir + "/" + target->GetName() + ".s_prefab";
					pref.CreateFromActor(target, WorldAdmin::GetSingleton()->GetActiveScene(), savePath);
				}

				ImGui::EndDragDropTarget();

			}
			JobsSystem::BeginSubmition();
			{
				DrawHierarchy();
			}
			JobsSystem::EndSubmition();
		}
		ImGui::End();
	}

	void FileExplorer::Import()
	{
		for (auto& p : std::filesystem::recursive_directory_iterator(m_CurrentDir))
		{
			if (std::filesystem::is_directory(p) == false)
			{
				std::filesystem::path path = p.path();
				std::string ext = path.extension().u8string();

				if (ext == ".jpg" || ext == ".png")
				{
					std::string new_path = path.parent_path().u8string() + "/" + path.stem().u8string() + ".s_image";
					if (std::filesystem::exists(new_path) == false)
					{
						TextureCreateInfo texInfo = {};
						texInfo.FilePath = path.u8string();
						texInfo.Save(new_path);
					}
				}

				if (ext == ".wav" || ext == ".mp3")
				{
					std::string new_path = path.parent_path().u8string() + "/" + path.stem().u8string() + ".s_audio";
					if (std::filesystem::exists(new_path) == false)
					{
						AudioClipCreateInfo info{};
						info.FilePath = path.u8string();;
						info.Save(new_path);
					}
				}
			}
			else
			{
				if (m_SelectedNode.empty())
				{
					m_SelectedNode = p.path().u8string();
				}
			}
		}
	}

	void FileExplorer::SetOnFileSelectedCallback(const std::function<void(const std::string&, const std::string&, int)>& callback)
	{
		m_pOnFileSelected = callback;
	}

	void FileExplorer::SetOnFileDeletedCallaback(const std::function<void(const std::string&, const std::string&)>& callback)
	{
		m_pOnFileDeleted = callback;
	}

	bool FileExplorer::DrawDirectory(const std::filesystem::path& orig_path)
	{
		const std::string fileName = orig_path.filename().u8string();
		const std::string filePath = orig_path.u8string();
		const bool isVisible = fileName.find(m_SearchBuffer) != std::string::npos;
		if (!isVisible)
			return false;

		DrawSelectable(fileName, filePath, "", true);
		if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
		{
			m_SelectedNode = filePath;
			if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			{
				m_CurrentDir = filePath;
				Reset();
				return true;
			}
		}
		ImGui::TextWrapped(fileName.c_str());
		ImGui::NextColumn();
		return false;
	}

	void FileExplorer::DrawNode(const std::filesystem::path& fs_path)
	{
		const std::string name = fs_path.filename().u8string();
		const std::string path = fs_path.u8string();
		const std::string ext = fs_path.extension().u8string();
		const bool isVisible = name.find(m_SearchBuffer) != std::string::npos;
		const bool extSupported = std::find(m_FileExtensions.begin(), m_FileExtensions.end(), ext) != m_FileExtensions.end();
		if (!extSupported || !isVisible)
			return;

		DrawSelectable(name, path, ext, false);
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
		{
			m_DragAndDropBuffer = path;
			ImGui::SetDragDropPayload("FileBrowser", &m_DragAndDropBuffer, sizeof(std::string));
			void* descriptorPtr = nullptr;
			GetIcon(path, ext, descriptorPtr);
			if (descriptorPtr != nullptr)
			{
				ImGui::Image(descriptorPtr, { m_ButtonSize.x, m_ButtonSize.y }, ImVec2(1, 1), ImVec2(0, 0));
			}
			ImGui::TextWrapped(name.c_str());
			ImGui::EndDragDropSource();
		}

		const bool is_action_pending = IsAnyActionPending(fs_path);
		if (is_action_pending == false)
		{
			if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
			{
				m_PopUpBuffer = path;
				m_ePopUpFlags = PopUpFlags::Node;
			}

			if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
			{
				if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					m_SelectedNode = path;
					if (m_pOnFileSelected != nullptr)
					{
						size_t fSize = GetNodeSize(path);
						m_pOnFileSelected(path, ext, static_cast<int>(fSize));
					}
				}
			}

			ImGui::TextWrapped(name.c_str());
			ImGui::NextColumn();
			return;
		}

		if (m_pPendeingAction->Type == PendeingActionFlags::NewFile || m_pPendeingAction->Type == PendeingActionFlags::Rename)
		{
			ImGui::PushStyleColor(ImGuiCol_FrameBg, { 0.180f, 0.521f, 1.0f,  1.0f });
			if (ImGui::InputTextWithHint("##ActionsRenameNode", name.c_str(), &m_pPendeingAction->NewName, ImGuiInputTextFlags_EnterReturnsTrue))
			{
				if (m_pPendeingAction->NewName.empty() == false)
				{
					std::filesystem::rename(path, fs_path.parent_path().u8string() + "/" + m_pPendeingAction->NewName + ext);
				}

				switch (m_pPendeingAction->Type)
				{
				case PendeingActionFlags::NewFile:
				{
					if (m_pPendeingAction->NewName.empty() == true)
					{
						std::filesystem::remove(path);
					}

					break;
				}
				}

				delete m_pPendeingAction;
				m_pPendeingAction = nullptr;
			}
			ImGui::PopStyleColor();
		}

		ImGui::NextColumn();
	}

	void FileExplorer::DrawIcon(const std::string& path, const std::string& ext)
	{
		void* descriptorPtr = nullptr;
		GetIcon(path, ext, descriptorPtr);

		if (descriptorPtr != nullptr)
		{
			ImGui::Image(descriptorPtr, { m_ButtonSize.x, m_ButtonSize.y });
		}
	}

	void FileExplorer::DrawToolBar()
	{
		{
			bool active = std::filesystem::equivalent(m_CurrentDir, m_HomeDir) == false;
			if (active == false) { ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f); }

			if (ImGui::ImageButton(m_pTextureLoader->m_ReturnIcon->GetImGuiTexture(), { 25, 25 }, ImVec2(0, 1), ImVec2(1, 0)))
			{
				if (active)
				{
					std::filesystem::path p(m_CurrentDir);
					m_CurrentDir = p.parent_path().u8string();
					Reset();
				}
			}

			if (active == false) { ImGui::PopStyleVar(); }
		}

		ImGui::SameLine();
		if (ImGui::ImageButton(m_pTextureLoader->m_ForwardIcon->GetImGuiTexture(), { 25, 25 }, ImVec2(0, 1), ImVec2(1, 0)))
		{
			if (m_SelectedNode.empty() == false)
			{
				m_CurrentDir = m_SelectedNode;
				Reset();
			}
		}

		ImGui::SameLine();
		if (ImGui::ImageButton(m_pTextureLoader->m_UpdateIcon->GetImGuiTexture(), { 25, 25 }, ImVec2(0, 1), ImVec2(1, 0)))
		{
			Import();
		}

		ImGui::SameLine();
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::PushTextWrapPos(450.0f);
			ImGui::TextUnformatted("import");
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}

		ImGui::SameLine();

		float pos = ImGui::GetCursorPosY();
		ImGui::SetCursorPosY(pos + 3);
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - (ImGui::GetContentRegionAvail().x / 1.4f));
		ImGui::InputTextWithHint("##Filter", "search...", &m_SearchBuffer);
		ImGui::SameLine();
		ImGui::Text(m_CurrentDir.c_str());
		ImGui::Separator();
	}

	void FileExplorer::DrawPopUp()
	{
		switch (m_ePopUpFlags)
		{
		case PopUpFlags::None:  if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) { ImGui::OpenPopup("FileExplorerActions"); }; break;
		case PopUpFlags::Node: 	if (m_pPendeingAction == nullptr) { ImGui::OpenPopup("FileExplorerNodeActions"); }; break;
		}

		if (ImGui::BeginPopup("FileExplorerActions"))
		{
			ImGui::MenuItem("actions", NULL, false, false);
			ImGui::Separator();

			if (ImGui::MenuItem("new material"))
			{
				std::string newPath = m_CurrentDir + "/new_material" + ".s_material";
				AddPendingAction(newPath, PendeingActionFlags::NewFile);
				ClosePopUp();
			}

			if (ImGui::MenuItem("new animation"))
			{
				std::string newPath = m_CurrentDir + "/new_animation" + ".s_animation";
				AddPendingAction(newPath, PendeingActionFlags::NewFile);
				ClosePopUp();
			}

			if (ImGui::MenuItem("new shader"))
			{
				ClosePopUp();
			}

			m_ePopUpFlags = PopUpFlags::None;
			ImGui::EndPopup();
		}

		if (ImGui::BeginPopup("FileExplorerNodeActions"))
		{
			ImGui::MenuItem("actions", NULL, false, false);
			ImGui::Separator();

			if (ImGui::MenuItem("rename"))
			{
				AddPendingAction(m_PopUpBuffer, PendeingActionFlags::Rename);
				ClosePopUp();
			}

			if (ImGui::BeginMenu("delete"))
			{
				std::filesystem::path p(m_PopUpBuffer);
				ImGui::NewLine();
				ImGui::TextWrapped("delete %s?", p.filename().stem().u8string().c_str());
				ImGui::SameLine();
				float posY = ImGui::GetCursorPosY();
				ImGui::SetCursorPosY(posY - 10);
				if (ImGui::Button("confirm", { 70, 35 }))
				{
					if (m_pOnFileDeleted != nullptr)
						m_pOnFileDeleted(m_PopUpBuffer, p.extension().u8string());

					std::filesystem::remove(m_PopUpBuffer);
					ClosePopUp();
				}

				ImGui::EndMenu();
			}

			m_ePopUpFlags = PopUpFlags::None;
			ImGui::EndPopup();
		}
	}

	void FileExplorer::AddPendingAction(const std::string& path, PendeingActionFlags action)
	{
		m_pPendeingAction = new PendeingAction();
		m_pPendeingAction->Type = action;
		m_pPendeingAction->Path = path;

		switch (action)
		{
		case PendeingActionFlags::NewFile:
		{
			if (std::filesystem::exists(path) == false)
			{
				std::ofstream outfile(m_pPendeingAction->Path);
				outfile.close();
				return;
			}

			break;
		}
		case PendeingActionFlags::Rename: return;
		}

		delete m_pPendeingAction;
		m_pPendeingAction = nullptr;
	}

	bool FileExplorer::IsAnyActionPending(const std::filesystem::path& node_path)
	{
		if (m_pPendeingAction != nullptr)
		{
			std::filesystem::path p1(m_pPendeingAction->Path);
			std::string f1 = p1.filename().u8string();
			std::string f2 = node_path.filename().u8string();

			return f1 == f2;
		}

		return false;
	}

	size_t FileExplorer::GetNodeSize(const std::string& path)
	{
		std::ifstream in(path, std::ifstream::ate | std::ifstream::binary);
		return in.tellg();
	}

	void FileExplorer::GetIcon(const std::string& path, const std::string& ext, void*& descriptorPtr)
	{
		if (ext == ".s_image")
		{
			TextureCreateInfo texCI = {};
			texCI.Load(path);
			texCI.bImGUIHandle = true;
			bool is_ktx = texCI.FilePath.find("ktx") != std::string::npos;

			if (is_ktx == false)
			{
				auto& it = m_IconsMap.find(path);
				if (it == m_IconsMap.end())
				{
					auto icon = Texture::Create();
					m_IconsMap[path] = icon;
					JobsSystem::Schedule([this, path, texCI]()
					{
						TextureCreateInfo info = texCI;
						auto& tex = m_IconsMap[path];
						tex = Texture::Create();
						tex->LoadFromFile(&info);
					});
				}
				else 
				{ 
					descriptorPtr = it->second->GetImGuiTexture();
				}
			}
			else
			{
				descriptorPtr = m_pTextureLoader->m_DocumentsIcon->GetImGuiTexture();
			}
		}
		else if (ext == ".s_material")
		{
			descriptorPtr = m_pTextureLoader->m_DocumentsIcon->GetImGuiTexture();
		}
		else if (ext == ".s_scene")
		{
			descriptorPtr = m_pTextureLoader->m_SceneIcon->GetImGuiTexture();
		}
		else if (ext == ".s_prefab")
		{
			descriptorPtr = m_pTextureLoader->m_BlueprintIcon->GetImGuiTexture();
		}
		else if (ext == ".s_audio")
		{
			descriptorPtr = m_pTextureLoader->m_AudioFileIcon->GetImGuiTexture();
		}
		else if (ext == ".s_animation")
		{
			descriptorPtr = m_pTextureLoader->m_AnimationIcon->GetImGuiTexture();
		}
		else if (ext == ".gltf")
		{
			descriptorPtr = m_pTextureLoader->m_glTFIcon->GetImGuiTexture();
		}
		else
		{
			descriptorPtr = m_pTextureLoader->m_DocumentsIcon->GetImGuiTexture();
		}
	}

	void FileExplorer::ClosePopUp()
	{
		ImGui::CloseCurrentPopup();
		m_PopUpBuffer = "";
	}

	void FileExplorer::Reset()
	{
		m_IconsMap.clear();
		m_SelectedNode.clear();
		m_PopUpBuffer.clear();

		if (m_pPendeingAction != nullptr)
		{
			delete m_pPendeingAction;
			m_pPendeingAction = nullptr;
		}

		ImGui::CloseCurrentPopup();
	}

	void FileExplorer::DrawHierarchy()
	{
		DrawToolBar();

		static float padding = 24.0f;
		float cellSize = m_ButtonSize.x + padding;

		float panelWidth = ImGui::GetContentRegionAvail().x;
		int columnCount = (int)(panelWidth / cellSize);
		if (columnCount < 1)
			columnCount = 1;

		ImGui::Columns(columnCount, 0, false);
		// first draw directories
		for (auto& file_path : std::filesystem::directory_iterator(m_CurrentDir))
		{
			if (std::filesystem::is_directory(file_path))
			{
				auto p = file_path.path();
				if (DrawDirectory(p))
				{
					ImGui::Columns(1);
					ImGui::SetWindowFontScale(1.0f);
					DrawPopUp();
					return;
				}
			}
		}
		ImGui::SetWindowFontScale(0.9f);
		for (auto& file_path : std::filesystem::directory_iterator(m_CurrentDir))
		{
			if (std::filesystem::is_directory(file_path) == false)
			{
				auto p = file_path.path();
				DrawNode(p);
			}
		}

		ImGui::Columns(1);
		ImGui::SetWindowFontScale(1.0f);
		DrawPopUp();
	}

	void FileExplorer::DrawSelectable(const std::string& name, const std::string& path, const std::string& ext, bool is_folder)
	{
		ImGui::PushID(path.c_str());
		void* descriptorPtr = m_pTextureLoader->m_FolderButton->GetImGuiTexture();
		if(!is_folder)
			GetIcon(path, ext, descriptorPtr);

		bool selected = m_SelectedNode == path;
		if (selected) { ImGui::PushStyleColor(ImGuiCol_Text, m_SelectColor); }
		else { ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0)); }
		{
			ImGui::ImageButton(descriptorPtr, { m_ButtonSize.x, m_ButtonSize.y }, ImVec2(1, 1), ImVec2(0, 0));
		}

		ImGui::PopStyleColor();
		ImGui::PopID();
	}
}