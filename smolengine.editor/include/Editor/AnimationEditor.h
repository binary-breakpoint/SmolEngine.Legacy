#pragma once
#include "Core/Core.h"
#include <string>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui_node_editor.h>

#ifndef FROSTIUM_SMOLENGINE_IMPL
#define FROSTIUM_SMOLENGINE_IMPL
#endif
#include <frostium/Animation/AnimationController.h>

namespace SmolEngine
{
	namespace ed = ax::NodeEditor;

	struct AnimationControllerComponent;

	class AnimationEditor
	{
	public:
		AnimationEditor();
		~AnimationEditor() = default;
		void Update(bool& is_open, AnimationControllerComponent* comp);
		void Reset();

	private:
		void DrawToolBar(AnimationControllerComponent* comp);
		void DrawNodeEditor(AnimationControllerComponent* comp);
		void DrawPopUp(AnimationControllerComponent* comp);
		void ClosePopUp();
		void CheckAndOpenPopUp();

	private:
		ed::EditorContext*      m_Context = nullptr;
		std::string             m_SelectedNode = "";
		std::string             m_Buffer = "";
		bool                    m_bOpenPopUp = false;
	};
}