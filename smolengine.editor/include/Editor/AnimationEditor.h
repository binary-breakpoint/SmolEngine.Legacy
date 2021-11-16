#pragma once
#include "Core/Core.h"
#include <string>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui_node_editor.h>

#include "Animation/AnimationController.h"

namespace SmolEngine
{
	namespace ed = ax::NodeEditor;

	struct MeshComponent;

	class AnimationEditor
	{
	public:
		AnimationEditor();
		~AnimationEditor() = default;
		void Update(bool& is_open, MeshComponent* comp);
		void Reset();

	private:
		void DrawToolBar(MeshComponent* comp);
		void DrawNodeEditor(MeshComponent* comp);
		void DrawPopUp(MeshComponent* comp);
		void ClosePopUp();
		void CheckAndOpenPopUp();

	private:
		ed::EditorContext*      m_Context = nullptr;
		std::string             m_SelectedNode = "";
		std::string             m_Buffer = "";
		bool                    m_bOpenPopUp = false;
	};
}