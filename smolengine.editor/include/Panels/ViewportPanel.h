#pragma once
#include "Core/Core.h"
#include "TexturesLoader.h"

#include <imgui/imgui.h>
#include <glm/glm.hpp>

#ifndef FROSTIUM_SMOLENGINE_IMPL
#define FROSTIUM_SMOLENGINE_IMPL
#endif
#include <frostium/External/imgizmo/src/ImGuizmo.h>
#include <frostium/Primitives/Framebuffer.h>

namespace SmolEngine
{
	class Actor;
	class EditorLayer;
	class WorldAdmin;
	class EditorCamera;
	struct GraphicsEngineSComponent;

	class ViewportPanel
	{
	public:
		ViewportPanel();
		virtual ~ViewportPanel() = default;

		void                SetActive(bool value) { m_Active = value; }
		const glm::vec2&    GetSize() const { return m_ViewPortSize; }
		bool                IsFocused() const { return m_Focused; }
		bool                IsActive() const { return m_Active; }
		virtual void        OnEvent(Event& e) {};
		virtual void        OnUpdate(float delta) {}
		virtual void        Render() {}
		virtual void        Draw() = 0;

	private:
		WorldAdmin*         m_World = nullptr;
		bool                m_Focused = false;
		bool                m_Active = false;
		glm::ivec2          m_ViewPortSize = glm::vec2(0);

		friend class SceneView;
		friend class GameView;
	};

	class SceneView : public ViewportPanel
	{
	public:
		SceneView(EditorLayer* editor);

		void                  OnUpdate(float delta) override;
		void                  OnEvent(Event& e) override;
		void                  Draw() override;

	private:
		void                  DrawViewPort();
		void                  DrawToolBar(Ref<Actor>& selectedActor);
		void                  DrawDragAndDrop(Ref<Actor>& selectedActor);
		void                  DrawGizmos(Ref<Actor>& selectedActor);

	private:
		bool                      m_GizmosEnabled = true;
		bool                      m_SnapEnabled = false;
		TexturesLoader*           m_TexturesLoader = nullptr;
		EditorCamera*             m_Camera = nullptr;
		EditorLayer*              m_Editor = nullptr;
		GraphicsEngineSComponent* m_GraphicsEngine = nullptr;
		ImGuizmo::OPERATION       m_GizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
		int                       m_DebugComboIndex = 1;
		int                       m_DrawModeComboIndex = 0;
		ImVec2                    m_ButtonSize;
	};

	class GameView : public ViewportPanel
	{
	public:
		GameView();

		void                  Draw() override;
		void                  Render() override;

		Framebuffer           m_PreviewFramebuffer = {};
	};
}