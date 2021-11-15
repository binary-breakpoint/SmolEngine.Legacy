#pragma once

#include "SmolEngineCore.h"
#include "Panels/TexturePanel.h"
#include "Panels/ConsolePanel.h"
#include "Panels/ViewportPanel.h"
#include "Panels/MaterialPanel.h"
#include "Panels/FileExplorer.h"
#include "Panels/AudioPanel.h"
#include "Panels/AnimationPanel.h"
#include "Editor-Tools/OzzTool.h"
#include "Editor/AnimationEditor.h"
#include "TexturesLoader.h"
#include "Layer/Layer.h"
#include "ECS/Components/BaseComponent.h"

#ifndef FROSTIUM_SMOLENGINE_IMPL
#define FROSTIUM_SMOLENGINE_IMPL
#endif
#include <frostium/Camera/EditorCamera.h>
#include <frostium/Primitives/Texture.h>

#include <imgui/imgui.h>
#include <glm/glm/glm.hpp>

namespace SmolEngine
{
	struct TransformComponent;
	struct Texture2DComponent;
	struct HeadComponent;
	struct Rigidbody2DComponent;
	struct CameraComponent;
	struct AudioSourceComponent;
	struct AnimationControllerComponent;
	struct CanvasComponent;
	struct Light2DSourceComponent;
	struct MeshComponent;
	struct PointLightComponent;
	struct SpotLightComponent;
	struct RigidbodyComponent;
	struct StaticbodyComponent;
	struct SkyLightComponent;
	struct PostProcessingComponent;
	struct DirectionalLightComponent;

	class Actor;
	class EditorCamera;

	enum class SelectionFlags: uint16_t
	{
		None = 0,
		Inspector,
		Actions,
		MaterialView,
		TextureView,
		AuidoView,
		AnimationView,
	};

	class EditorLayer: public Layer
	{
	public:

		EditorLayer(EditorCamera* camera)
			:m_Camera(camera), Layer("EditorLayer") {}

		~EditorLayer() = default;

		// Override
		void OnAttach() override;
		void OnBeginFrame(float deltaTime) override;
		void OnEndFrame(float deltaTime) override;
		void OnEvent(Event& event) override;
		void OnImGuiRender() override;

		// Draw
		void DrawActor(Ref<Actor>& actor, uint32_t index = 0);
		void DrawToolsBar();
		void DrawMeshPanel();
		void DrawInfo(HeadComponent* head);
		void DrawTransform(TransformComponent* transform);
		void DrawTexture(Texture2DComponent* texture);
		void DrawRigidBody2D(Rigidbody2DComponent* rb);
		void DrawCamera(CameraComponent* camera);
		void DrawAudioSource(AudioSourceComponent* audio);
		void DrawAnimationController(AnimationControllerComponent* anim);
		void DrawCanvas(CanvasComponent* canvas);
		void DrawLight2D(Light2DSourceComponent* light);
		void DrawInspector();
		void DrawHierarchy();
		void DrawMeshComponent(MeshComponent* meshComponent);
		void DrawPointLightComponent(PointLightComponent* light);
		void DrawSpotLightComponent(SpotLightComponent* light);
		void DrawRigidBodyComponent(RigidbodyComponent* component);
		void DrawPostProcessingComponent(PostProcessingComponent* component);
		void DrawSkyLightComponent(SkyLightComponent* component);
		void DrawDirectionalLightComponent(DirectionalLightComponent* component);

		void DrawComponents();
		void DrawComponentPopUp();
		void DrawScriptPopUp();
		void CheckSelectedActor();

	private:
		template<typename T>
		bool IsCurrentComponent(uint32_t index)
		{
			if (m_World->GetActiveScene()->HasComponent<T>(m_SelectedActor))
			{
				auto comp = m_World->GetActiveScene()->GetComponent<T>(m_SelectedActor);
				BaseComponent* baseComp = dynamic_cast<BaseComponent*>(comp);
				return baseComp->ComponentID == index;
			}

			return false;
		}

		void ResetSelection();
		void DrawScriptComponent(uint32_t index);
		void DrawMeshPrimitive(uint32_t type, const std::string& title, const std::string& desc, Texture* icon);
		void CheckActor(Ref<Actor>& actor);

		// Callbacks
		void OnFileSelected(const std::string& path, const std::string& ext, int fileSize);
		void OnFileDeleted(const std::string& path, const std::string& ext);

	private:
		AnimationEditor*              m_AnimationEditor = nullptr;
		TexturesLoader*               m_TexturesLoader = nullptr;
		OzzTool*                      m_OzzTool = nullptr;
		FileExplorer*                 m_FileExplorer = nullptr;
		WorldAdmin*                   m_World = nullptr;
		ViewportPanel*                m_SceneView = nullptr;
		ViewportPanel*                m_GameView = nullptr;
		AudioPanel*                   m_AudioPanel = nullptr;
		EditorCamera*                 m_Camera = nullptr;
		ConsolePanel*                 m_Console = nullptr;
		TexturePanel*                 m_TextureInspector = nullptr;
		AnimationPanel*               m_AnimationPanel = nullptr;
		Ref<Actor>                    m_SelectedActor = nullptr;
		MaterialPanel*                m_MaterialInspector = nullptr;
		SelectionFlags                m_SelectionFlags = SelectionFlags::None;
		std::string                   m_FilePath = "";
		std::string                   m_FileName = "";
		uint32_t                      m_IDBuffer = 0;
		std::vector<Ref<Actor>>       m_DisplayedActors;
		bool                          m_bShowConsole = true;
		bool                          m_bShowGameView = false;
		bool                          m_bShowSettings = false;
		bool                          m_bShowAnimEditor = true;
		bool                          m_bShowOzzTool = false;
		std::string                   m_TempActorName = "";
		std::string                   m_TempActorTag = "";
		std::string                   m_TempString = "";               

		friend class MaterialPanel;
		friend class SceneView;
	};
}

