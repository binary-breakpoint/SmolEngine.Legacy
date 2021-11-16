#include "stdafx.h"
#include "EditorLayer.h"
#include "ImGuiExtension.h"
#include "Editor-Tools/Tools.h"

#include "ECS/WorldAdmin.h"
#include "ECS/Actor.h"
#include "ECS/Components/Include/Components.h"
#include "ECS/Systems/RendererSystem.h"
#include "ECS/Systems/Physics2DSystem.h"
#include "ECS/Systems/AudioSystem.h"
#include "ECS/Systems/UISystem.h"
#include "ECS/Systems/ScriptingSystem.h"
#include "ECS/Components/Singletons/Box2DWorldSComponent.h"
#include "ECS/Components/Singletons/ProjectConfigSComponent.h"
#include "ECS/Components/Singletons/GraphicsEngineSComponent.h"
#include "ECS/Scene.h"
#include "ECS/Prefab.h"

#include "Tools/GLM.h"
#include "Tools/Utils.h"
#include "Scripting/CSharp/MonoContext.h"
#include "Scripting/CPP/MetaContext.h"

namespace SmolEngine
{
	void EditorLayer::OnAttach()
	{
		m_OzzTool = new OzzTool();
		m_AnimationEditor = new AnimationEditor();
		m_Console = new ConsolePanel();
		m_TexturesLoader = new TexturesLoader();
		m_TextureInspector = new TexturePanel();
		m_MaterialInspector = new MaterialPanel();
		m_AnimationPanel = new AnimationPanel();
		m_AudioPanel = new AudioPanel();
		m_World = WorldAdmin::GetSingleton();
		m_World->CreateScene(std::string("TestScene2.s_scene"));

		m_SceneView = new SceneView(this);
		m_GameView = new GameView();

		m_FileExplorer = new FileExplorer();
		m_FileExplorer->Create(Engine::GetEngine()->GetAssetsFolder());
		m_FileExplorer->SetOnFileSelectedCallback(std::bind_front(&EditorLayer::OnFileSelected, this));
		m_FileExplorer->SetOnFileDeletedCallaback(std::bind_front(&EditorLayer::OnFileDeleted, this));

		{
			ImGui::GetStyle().FrameRounding = 4.0f;
			ImGui::GetStyle().GrabRounding = 4.0f;

			ImVec4* colors = ImGui::GetStyle().Colors;
			colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
			colors[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.42f, 0.47f, 1.00f);
			colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
			colors[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
			colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
			colors[ImGuiCol_Border] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
			colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
			colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
			colors[ImGuiCol_FrameBgHovered] = ImVec4(0.12f, 0.20f, 0.28f, 1.00f);
			colors[ImGuiCol_FrameBgActive] = ImVec4(0.09f, 0.12f, 0.14f, 1.00f);
			colors[ImGuiCol_TitleBg] = ImVec4(0.09f, 0.12f, 0.14f, 0.65f);
			colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
			colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.5f);
			colors[ImGuiCol_MenuBarBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
			colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.39f);
			colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
			colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.18f, 0.22f, 0.25f, 1.00f);
			colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.09f, 0.21f, 0.31f, 1.00f);
			colors[ImGuiCol_CheckMark] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
			colors[ImGuiCol_SliderGrab] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
			colors[ImGuiCol_SliderGrabActive] = ImVec4(0.37f, 0.61f, 1.00f, 1.00f);
			colors[ImGuiCol_Header] = ImVec4(0.20f, 0.25f, 0.29f, 0.55f);
			colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
			colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
			colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
			colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
			colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
			colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
			colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
			colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
			colors[ImGuiCol_Tab] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
			colors[ImGuiCol_TabHovered] = ImVec4(0.1f, 0.1f, 0.1f, 0.1f);
			colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
			colors[ImGuiCol_TabUnfocused] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
			colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
			colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
			colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
			colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
			colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
			colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
			colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
			colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
			colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
			colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
			colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

			// Buttons
			colors[ImGuiCol_Button] = ImVec4(0.21f, 0.68f, 0.80f, 1.00f);
			colors[ImGuiCol_ButtonHovered] = ImVec4(0.45f, 0.70f, 0.82f, 1.00f);
			colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);

			colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
			colors[ImGuiCol_DockingPreview] = ImVec4(0.85f, 0.85f, 0.85f, 0.28f);
		}

		DebugLog::SetCallback([&](const std::string&& msg, LogLevel type)
		{
			switch (type)
			{
			case LogLevel::Info:
			{
				m_Console->AddMessageInfo(msg);
				break;
			}
			case LogLevel::Warning:
			{
				m_Console->AddMessageWarn(msg);
				break;
			}
			case LogLevel::Error:
			{
				m_Console->AddMessageError(msg);
				break;
			}
			}
		});

#if 0
		auto actor = m_World->GetActiveScene()->CreateActor();
		auto audio =m_World->GetActiveScene()->AddComponent<AudioSourceComponent>(actor);
		AudioClipCreateInfo soundCI{};
		soundCI.eType = SoundType::Sound_3D;
		soundCI.FilePath = "../samples/audio/test_audio.wav";
		ComponentHandler::ValidateAudioSourceComponent(audio, &soundCI);

		Ref<AudioHandle> handle = nullptr;
		AudioSystem::PlayClip(&audio->GetClipByIndex(0)->m_Clip, handle);
#endif
	}

	void EditorLayer::OnBeginFrame(float deltaTime)
	{
		m_SceneView->OnUpdate(deltaTime);

		auto frostium = GraphicsEngineSComponent::Get();
		frostium->ViewProj.Update(m_Camera);
	}

	void EditorLayer::OnEndFrame(float deltaTime)
	{
		if (m_GameView->IsActive())
			m_GameView->Render();
	}

	void EditorLayer::OnEvent(Event& e)
	{
		m_SceneView->OnEvent(e);
	}

	void EditorLayer::OnImGuiRender()
	{
		CheckSelectedActor();

		static bool p_open = true;
		static bool opt_fullscreen_persistant = true;
		bool opt_fullscreen = opt_fullscreen_persistant;
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		if (opt_fullscreen)
		{
			ImGuiViewport* ViewportPanel = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(ViewportPanel->WorkPos);
			ImGui::SetNextWindowSize(ViewportPanel->WorkSize);
			ImGui::SetNextWindowViewport(ViewportPanel->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		}

		if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
			window_flags |= ImGuiWindowFlags_NoBackground;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpace", &p_open, window_flags);
		ImGui::PopStyleVar();

		if (opt_fullscreen)
			ImGui::PopStyleVar(2);

		// DockSpace
		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
		}

		//ImGui::ShowDemoWindow();

		MonoContext::GetSingleton()->Track();
		m_SceneView->Draw();
		m_GameView->Draw();

		m_Console->Update(m_bShowConsole);
		m_OzzTool->Update(m_bShowOzzTool);
		m_FileExplorer->Update();

		DrawToolsBar();
		DrawHierarchy();
		DrawInspector();

		ImGui::End();

	}

	void EditorLayer::DrawActor(Ref<Actor>& actor, uint32_t index)
	{
		if (m_SelectedActor == actor)
			ImGui::PushStyleColor(ImGuiCol_Text, { 0.1f, 0.3f, 1.0f, 1.0f });

		bool open = ImGui::TreeNodeEx(actor->GetName().c_str(), m_SelectedActor == actor ? ImGuiTreeNodeFlags_Selected | ImGuiTreeNodeFlags_OpenOnArrow : ImGuiTreeNodeFlags_OpenOnArrow);
		if (m_SelectedActor == actor)
			ImGui::PopStyleColor();

		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
		{
			m_IDBuffer = actor->GetID();
			ImGui::SetDragDropPayload("ActorDragAndDrop", &m_IDBuffer, sizeof(Actor));
			ImGui::Text(actor->GetName().c_str());
			ImGui::EndDragDropSource();
			m_IDBuffer = index;
		}

		if (ImGui::IsItemClicked(1))
		{
			ResetSelection();
			m_SelectionFlags = SelectionFlags::Actions;
			m_SelectedActor = actor;
		}

		if (ImGui::IsItemClicked())
		{
			if (ImGui::IsMouseDoubleClicked(0))
			{
				ResetSelection();
				m_SelectionFlags = SelectionFlags::Inspector;
				m_SelectedActor = actor;
			}
		}

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ActorDragAndDrop"))
			{
				uint32_t id = *static_cast<uint32_t*>(payload->Data);
				Ref<Actor> target = m_World->GetActiveScene()->FindActorByID(id);
				if (target != actor)
				{
					actor->SetChild(target);
				}
			}

			ImGui::EndDragDropTarget();
		}

		if (open)
		{
			uint32_t i = 0;
			for (auto child : actor->GetChilds())
			{
				DrawActor(child, i);
				i++;
			}

			ImGui::TreePop();
		}
	}

	void EditorLayer::DrawToolsBar()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 10.0f, 10.0f });
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Close"))
				{
					Engine::GetEngine()->Shutdown();
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Scene"))
			{
				if (!m_World->IsInPlayMode())
				{
					if (ImGui::MenuItem("New"))
					{
						const auto& result = Utils::SaveFile("SmolEngine Scene (*.s_scene)\0*.s_scene\0", "new_scene.s_scene");
						if (result.has_value())
						{
							m_SelectedActor = nullptr;
							m_World->CreateScene(result.value());
						}
					}

					if (ImGui::MenuItem("Save"))
					{
						if (!m_World->SaveCurrentScene())
						{
							DebugLog::LogError("Couldn't save current scene!");
						}
					}

					if (ImGui::MenuItem("Save as"))
					{
						const auto& result = Utils::SaveFile("SmolEngine Scene (*.s_scene)\0*.s_scene\0", "new_scene.s_scene");
						if (result.has_value())
						{
							m_SelectedActor = nullptr;
							std::filesystem::path path = result.value();
							m_World->GetActiveScene()->GetSceneState()->Name = path.filename().stem().string();
							m_World->SaveScene(result.value());
						}
					}

					if (ImGui::MenuItem("Load"))
					{
						const auto& result = Utils::OpenFile("SmolEngine Scene (*.s_scene)\0*.s_scene\0");
						if (result.has_value())
						{
							m_SelectedActor = nullptr;
							m_World->LoadScene(result.value());
						}
					}
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Window"))
			{
				if (ImGui::MenuItem("Console"))
					m_bShowConsole = true;

				if (ImGui::MenuItem("Preview"))
					m_GameView->SetActive(true);

				if (ImGui::MenuItem("Settings"))
					m_bShowSettings = true;

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Tools"))
			{
				if (ImGui::MenuItem("Ozz-Converter"))
					m_bShowOzzTool = true;

				if (ImGui::MenuItem("Update Materials"))
					m_World->ReloadMaterials();

				ImGui::EndMenu();
			}

		}
		ImGui::EndMainMenuBar();
		ImGui::PopStyleVar();

	}

	void EditorLayer::DrawMeshPanel()
	{
		ImGui::BeginChild("CreationTools");
		{
			ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg;
			if (ImGui::BeginTable("PrimitivesView", 3, flags))
			{
				ImGui::TableSetupColumn("Mesh", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("Preview", ImGuiTableColumnFlags_NoHide);
				ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_NoHide);
				ImGui::TableHeadersRow();

				DrawMeshPrimitive((uint32_t)MeshTypeEX::Cube, "Cube", "Creates a new entity and attaches a cube mesh to it.", m_TexturesLoader->m_CubeIcon);
				DrawMeshPrimitive((uint32_t)MeshTypeEX::Sphere, "Sphere", "Creates a new entity and attaches a sphere mesh to it.", m_TexturesLoader->m_SphereIcon);
				DrawMeshPrimitive((uint32_t)MeshTypeEX::Capsule, "Capsule", "Creates a new entity and attaches a capsule mesh to it.", m_TexturesLoader->m_CapsuleIcon);
				DrawMeshPrimitive((uint32_t)MeshTypeEX::Torus, "Torus", "Creates a new entity and attaches a torus mesh to it.", m_TexturesLoader->m_TorusIcon);

				ImGui::EndTable();
			}
		}
		ImGui::EndChild();
	}

	void EditorLayer::DrawInfo(HeadComponent* head)
	{
		m_TempActorName = head->Name;
		m_TempActorTag = head->Tag;

		if (ImGui::Extensions::InputRawString("Name", m_TempActorName))
			m_SelectedActor->SetName(m_TempActorName);

		ImGui::Extensions::InputString("Tag", head->Tag, m_TempActorTag);
		ImGui::Extensions::CheckBox("Enabled", head->bEnabled);
	}

	void EditorLayer::DrawTransform(TransformComponent* transform)
	{
		ImGui::Extensions::TransformComponent(transform->WorldPos, transform->Scale, transform->Rotation);
	}

	void EditorLayer::DrawTexture(Texture2DComponent* texture)
	{
		if (texture->Texture != nullptr)
		{
			ImGui::Extensions::ColorInput4("Color", texture->Color);
			ImGui::Extensions::InputInt("Layer", texture->LayerIndex);
			ImGui::NewLine();
			ImGui::Extensions::CheckBox("Enabled", texture->Enabled);
		}

		ImGui::NewLine();
		ImGui::SetCursorPosX(10);
		if (ImGui::Button("Load Texture", { ImGui::GetWindowWidth() - 20.0f, 30.0f }))
		{
			const auto& result = Utils::OpenFile("png (*png)\0*.png\0jpg (*jpg)\0*.jpg\0");
			if (result.has_value())
				texture->Load(result.value());
		}

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FileBrowser"))
			{
				std::string& path = *(std::string*)payload->Data;
				if (Tools::FileExtensionCheck(path, ".png") || Tools::FileExtensionCheck(path, ".jpg"))
				{
					texture->Load(path);
				}
			}
			ImGui::EndDragDropTarget();
		}

	}

	void EditorLayer::DrawRigidBody2D(Rigidbody2DComponent* rb)
	{
		ImGui::Extensions::Combo("Type", "Static\0Kinematic\0Dynamic\0\0", rb->Body.m_Type);
		ImGui::Extensions::Combo("Shape", "Box\0Circle\0\0", rb->Body.m_ShapeType);
		ImGui::Extensions::InputInt("Layer", rb->Body.m_CollisionLayer);
		ImGui::NewLine();

		if (rb->Body.m_ShapeType == (int)Shape2DType::Box)
		{
			ImGui::Extensions::InputFloat2("Size", rb->Body.m_Shape);
			ImGui::NewLine();
		}

		if (rb->Body.m_ShapeType == (int)Shape2DType::Cirlce)
		{
			ImGui::Extensions::InputFloat("Radius", rb->Body.m_Radius);
			ImGui::Extensions::InputFloat2("Offset", rb->Body.m_Offset);
			ImGui::NewLine();
		}


		if (rb->Body.m_Type == 2 || rb->Body.m_Type == 1)
		{
			ImGui::Extensions::InputFloat("Inertia Moment", rb->Body.m_InertiaMoment);
			ImGui::Extensions::InputFloat("Gravity", rb->Body.m_GravityScale);
			ImGui::Extensions::InputFloat("Mass", rb->Body.m_Mass);
			ImGui::Extensions::InputFloat2("Mass Center", rb->Body.m_MassCenter);
			ImGui::NewLine();
			ImGui::Extensions::InputFloat("Restitution", rb->Body.m_Restitution);
			ImGui::Extensions::InputFloat("Friction", rb->Body.m_Friction);
			ImGui::Extensions::InputFloat("Density", rb->Body.m_Density);
			ImGui::Extensions::CheckBox("Bullet", rb->Body.m_IsBullet);
			ImGui::NewLine();

		}

		ImGui::Extensions::CheckBox("Trigger", rb->Body.m_IsTrigger);
		ImGui::Extensions::CheckBox("Awake", rb->Body.m_IsAwake);
		ImGui::Extensions::CheckBox("Allow Sleep", rb->Body.m_canSleep);
		ImGui::Extensions::CheckBox("Draw Shape", rb->ShowShape);
	}

	void EditorLayer::DrawCamera(CameraComponent* camera)
	{
		if (ImGui::Extensions::Combo("Type", "Perspective\0Ortho\0", camera->ImGuiType))
			camera->eType = (CameraType)camera->ImGuiType;

		ImGui::Extensions::InputFloat("Zoom", camera->ZoomLevel);
		ImGui::Extensions::InputFloat("FOV", camera->FOV);
		ImGui::Extensions::InputFloat("Near", camera->zNear);
		ImGui::Extensions::InputFloat("Far", camera->zFar);

		ImGui::NewLine();
		if(ImGui::Extensions::CheckBox("Primary", camera->bPrimaryCamera))
		{
			if (camera->bPrimaryCamera)
			{
				const size_t id = m_SelectedActor->GetID();
				entt::registry& reg = m_World->GetActiveScene()->GetRegistry();
				reg.view<HeadComponent, CameraComponent>().each([&](HeadComponent& head, CameraComponent& camera)
				{
					if (head.ActorID != id)
						camera.bPrimaryCamera = false;
				});
			}
		}
	}

	void EditorLayer::DrawAudioSource(AudioSourceComponent* audio)
	{
		if (ImGui::BeginTabBar("AudioSourceTab"))
		{
			if (ImGui::BeginTabItem("Mixer"))
			{
				ImGui::PushID("MixeTabID");

				ImGui::NewLine();
				ImGui::SetCursorPosX(6.0f);
				if (ImGui::ImageButton(m_TexturesLoader->m_PlayButton->GetImGuiTexture(), ImVec2(15, 15)))
				{
					audio->StopAll();
					audio->PlayAll();
				}

				ImGui::SameLine();
				if (ImGui::ImageButton(m_TexturesLoader->m_StopButton->GetImGuiTexture(), ImVec2(15, 15)))
				{
					audio->StopAll();
				}

				float* ftt = audio->GetFFT();
				ImGui::PlotHistogram("##Mixer", ftt, 256 / 2, 0, "FFT", 0, 10, ImVec2(ImGui::GetContentRegionAvail().x, 80), 8);
				float* wave = audio->GetWave();
				ImGui::PlotLines("##Wave", wave, 256, 0, "Wave", -1, 1, ImVec2(ImGui::GetContentRegionAvail().x, 80));
				ImGui::NewLine();

				if (ImGui::Extensions::Slider("Mixer Volume", audio->Volume, 0.0f, 10.0f))
				{
					audio->SetVolume(audio->Volume);
				}
				if (ImGui::Extensions::Slider("Mixer Speed", audio->Speed, 0.0f, 10.0f))
				{
					audio->SetSpeed(audio->Speed);
				}

				ImGui::PopID();

				{
					ImGui::NewLine();
					ImGui::SetCursorPosX(6.0f);
					ImGuiTableFlags flags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody;
					if (ImGui::BeginTable("ClipView", 3, flags))
					{
						ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_WidthFixed);
						ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
						ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_NoHide);
						ImGui::TableHeadersRow();

						for (uint32_t i = 0; i < static_cast<uint32_t>(audio->Clips.size()); ++i)
						{
							auto& info = audio->Clips[i];
							std::string addID = "ClipID###" + std::to_string(i);
							ImGui::PushID(addID.c_str());

							ImGui::TableNextRow();
							ImGui::TableNextRow();
							ImGui::TableNextColumn();

							ImGui::TextWrapped(std::to_string(i).c_str());
							ImGui::TableNextColumn();

							ImGui::TextWrapped(info.m_CreateInfo.FileName.c_str());
							ImGui::TableNextColumn();

							if (ImGui::ImageButton(m_TexturesLoader->m_PlayButton->GetImGuiTexture(), ImVec2(15, 15)))
							{
								if (info.m_Handle)
								{
									audio->StopClip(info.m_Handle);
								}

								audio->PlayClip(info.m_Clip, info.m_Handle);
							}

							ImGui::SameLine();
							if (ImGui::ImageButton(m_TexturesLoader->m_StopButton->GetImGuiTexture(), ImVec2(15, 15)))
							{
								if (info.m_Handle)
								{
									audio->StopClip(info.m_Handle);
								}
							}

							ImGui::SameLine();
							if (ImGui::ImageButton(m_TexturesLoader->m_RemoveButton->GetImGuiTexture(), ImVec2(15, 15)))
							{
								if (info.m_Handle)
								{
									audio->StopClip(info.m_Handle);
								}

								audio->RemoveClipAtIndex(i);
							}

							ImGui::TableNextColumn();
							ImGui::PopID();
						}

						ImGui::EndTable();
					}

					ImGui::NewLine();
					ImGui::SetCursorPosX(10);
					ImGui::PushID("ClipID");
					ImGui::Button("Add", { ImGui::GetWindowWidth() - 25.0f, 25.0f });
					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FileBrowser"))
						{
							std::string& path = *(std::string*)payload->Data;
							if (Tools::FileExtensionCheck(path, ".s_audio"))
							{
								audio->LoadCip(path);
							}
						}
						ImGui::EndDragDropTarget();
					}

					ImGui::PopID();
				}

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Lo-fi"))
			{
				ImGui::NewLine();
				ImGui::PushID("LofiFilterID");

				auto& filter = audio->Filter;
				bool active = (filter.Flags & AudioFilterFlags::Lofi) == AudioFilterFlags::Lofi;
				if (ImGui::Extensions::CheckBox("Active", active))
				{
					if (active)
						audio->AddFilter(AudioFilterFlags::Lofi);
					else
						audio->RemoveFilter(AudioFilterFlags::Lofi);
				}

				ImGui::Extensions::InputFloat("SampleRate", filter.LofiCI.SampleRate);
				ImGui::Extensions::InputFloat("Bitdepthe", filter.LofiCI.Bitdepth);

				ImGui::PopID();
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Freeverb"))
			{
				ImGui::NewLine();
				ImGui::PushID("FreeverbFilterID");
				auto& filter = audio->Filter;
				bool active = (filter.Flags & AudioFilterFlags::Freeverb) == AudioFilterFlags::Freeverb;
				if (ImGui::Extensions::CheckBox("Active", active))
				{
					if (active)
						audio->AddFilter(AudioFilterFlags::Freeverb);
					else
						audio->RemoveFilter(AudioFilterFlags::Freeverb);
				}

				ImGui::Extensions::InputFloat("Damp", filter.FreeverbCI.Damp);
				ImGui::Extensions::InputFloat("Width", filter.FreeverbCI.Width);
				ImGui::Extensions::InputFloat("Room Size", filter.FreeverbCI.RoomSize);

				ImGui::PopID();
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Echo"))
			{
				ImGui::NewLine();
				ImGui::PushID("EchoFilterID");
				auto& filter = audio->Filter;
				bool active = (filter.Flags & AudioFilterFlags::Echo) == AudioFilterFlags::Echo;
				if (ImGui::Extensions::CheckBox("Active", active))
				{
					if (active)
						audio->AddFilter(AudioFilterFlags::Echo);
					else
						audio->RemoveFilter(AudioFilterFlags::Echo);
				}

				ImGui::Extensions::InputFloat("Decay", filter.EchoCI.Decay);
				ImGui::Extensions::InputFloat("Delay", filter.EchoCI.Delay);
				ImGui::Extensions::InputFloat("Filter", filter.EchoCI.Filter);

				ImGui::PopID();
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Bass-Boost"))
			{
				ImGui::NewLine();
				ImGui::PushID("BassBoostID");
				auto& filter = audio->Filter;
				bool active = (filter.Flags & AudioFilterFlags::Bassboost) == AudioFilterFlags::Bassboost;
				if (ImGui::Extensions::CheckBox("Active", active))
				{
					if (active)
						audio->AddFilter(AudioFilterFlags::Bassboost);
					else
						audio->RemoveFilter(AudioFilterFlags::Bassboost);
				}
				ImGui::Extensions::InputFloat("Boost", filter.BassboostCI.Boost);

				ImGui::PopID();
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}
	
		ImGui::NewLine();

	}

	void EditorLayer::DrawCanvas(CanvasComponent* canvas)
	{
		
	}

	void EditorLayer::DrawInspector()
	{
		ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_NoCollapse);
		{
			ImGui::BeginChild("InspectorChild");
			{
				ImGui::SetWindowFontScale(0.9f);
				if (m_SelectionFlags != SelectionFlags::Inspector)
				{
					if (m_SelectionFlags == SelectionFlags::MaterialView)
					{
						m_MaterialInspector->Update();
					}
					else if (m_SelectionFlags == SelectionFlags::AuidoView)
					{
						m_AudioPanel->Update();
					}
					else if (m_SelectionFlags == SelectionFlags::TextureView)
					{
						m_TextureInspector->Update();
					}
					else if (m_SelectionFlags == SelectionFlags::AnimationView)
					{
						m_AnimationPanel->Update();
					}
					else
					{
						DrawMeshPanel();
					}

					ImGui::EndChild();
					ImGui::End();
					return;
				}
				else if(m_SelectedActor && m_SelectionFlags == SelectionFlags::Inspector)
				{
					std::stringstream ss;

					if (ImGui::Button("Add Component"))
						ImGui::OpenPopup("AddComponentPopUp");

					ImGui::SameLine();
					if (ImGui::Button("Add C++ Script"))
					{
						ImGui::OpenPopup("AddCScriptPopUp");
					}

					ImGui::SameLine();
					if (ImGui::Button("Add C# Script"))
					{
						ImGui::OpenPopup("AddCSharpScriptPopUp");
					}

					DrawScriptPopUp();
					DrawComponentPopUp();

					DrawComponents();
				}
			}
			ImGui::EndChild();
		}

		ImGui::End();
	}

	void EditorLayer::DrawHierarchy()
	{
		ImGui::Begin("Hierarchy", nullptr, ImGuiWindowFlags_NoCollapse);
		{
			ImGui::SetWindowFontScale(0.8f);
			ImGui::Image(m_TexturesLoader->m_SearchButton->GetImGuiTexture(), { 25, 25 }, ImVec2(0, 1), ImVec2(1, 0));
			ImGui::SameLine();

			float pos = ImGui::GetCursorPosY();
			ImGui::SetCursorPosY(pos + 3);
			static char name[128];
			ImGui::InputTextWithHint("", "Search", name, IM_ARRAYSIZE(name));
			ImGui::Separator();

			std::string sceneStr = "Scene";
			SceneStateComponent* state = m_World->GetActiveScene()->GetSceneState();

			if (!state->Name.empty())
			{
				sceneStr = "Scene: " + state->Name;
			}

			ImGui::BeginChild("Scene");
			{
				if (ImGui::IsWindowHovered())
				{
					if (!ImGui::IsAnyItemHovered())
					{
						if (Input::IsMouseButtonPressed(MouseCode::Button1))
						{
							ImGui::OpenPopup("CreateActorPopUp");
						}

						if (Input::IsMouseButtonPressed(MouseCode::Button0))
						{
							m_SelectedActor = nullptr;
							m_SelectionFlags = SelectionFlags::None;
						}
					}
				}

				if (ImGui::BeginPopup("CreateActorPopUp"))
				{
					ImGui::MenuItem("New Actor", NULL, false, false);
					ImGui::Separator();
					std::stringstream ss;

					if (ImGui::MenuItem("Empty Actor"))
					{
						m_SelectedActor = m_World->GetActiveScene()->CreateActor();
						m_SelectionFlags = SelectionFlags::Inspector;
					}

					ImGui::EndPopup();
				}

				bool open = ImGui::TreeNodeEx(sceneStr.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ActorDragAndDrop"))
					{
						uint32_t id = *static_cast<uint32_t*>(payload->Data);
						Ref<Actor> parent = m_World->GetActiveScene()->FindActorByID(id)->GetParent();
						if (parent != nullptr)
							parent->RemoveChildAtIndex(m_IDBuffer);
						
						m_IDBuffer = 0;
					}

					ImGui::EndDragDropTarget();
				}

				if (open)
				{
					m_DisplayedActors.clear();
					m_World->GetActiveScene()->GetActors(m_DisplayedActors);

					for (auto obj : m_DisplayedActors)
						CheckActor(obj);

					for (auto& actor : m_DisplayedActors)
					{
						auto result = actor->GetName().find(name);
						if (result == std::string::npos)
						{
							continue;
						}

						DrawActor(actor);
					}

					ImGui::TreePop();
				}

				if (m_SelectionFlags == SelectionFlags::Actions)
				{
					ImGui::OpenPopup("ActionPopup");
					m_SelectionFlags = SelectionFlags::None;
				}

				if (ImGui::BeginPopup("ActionPopup"))
				{
					ImGui::MenuItem(m_SelectedActor->GetName().c_str(), NULL, false, false);
					ImGui::Separator();

					if (ImGui::MenuItem("Save as", "Ctrl+N")) 
					{
						if (m_SelectedActor)
						{
							const auto& result = Utils::SaveFile("SmolEngine Prefab (*.s_prefab)\0*.s_prefab\0", "new_prefab.s_prefab");
							if (result.has_value())
							{
								Prefab pref;
								pref.CreateFromActor(m_SelectedActor, m_World->GetActiveScene(), result.value());
							}
						}
					}

					if (ImGui::MenuItem("View", "Ctrl+V"))
					{
						glm::vec3 pos = m_SelectedActor->GetComponent<TransformComponent>()->WorldPos;
						m_Camera->SetDistance(6.0f);
						m_Camera->SetPosition(pos);
					}

					if (ImGui::MenuItem("Dublicate", "Ctrl+C"))
					{
						m_World->GetActiveScene()->DuplicateActor(m_SelectedActor);
					}

					if (ImGui::MenuItem("Delete", "Ctrl+E"))
					{
						m_World->GetActiveScene()->DeleteActor(m_SelectedActor);
						m_SelectedActor = nullptr;
					}

					ImGui::EndPopup();
				}
			}
			ImGui::EndChild();
		}
		ImGui::End();
	}

	void EditorLayer::DrawMeshComponent(MeshComponent* comp)
	{
		if (comp->GetMesh() != nullptr)
		{
			ImGui::Extensions::CheckBox("Show", comp->bShow);
			ImGui::Extensions::CheckBox("Static", comp->bIsStatic);
			ImGui::NewLine();

			ImGui::SetCursorPosX(6.0f);
			ImGuiTableFlags flags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody;
			if (ImGui::BeginTable("MeshView", 3, flags))
			{
				ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
				ImGui::TableSetupColumn("Material", ImGuiTableColumnFlags_NoHide);
				ImGui::TableHeadersRow();

				auto& scene = comp->GetMesh()->GetScene();
				for (uint32_t i = 0; i < static_cast<uint32_t>(scene.size()); ++i)
				{
					auto& node = scene[i];
					std::string addID = "MeshID###" + std::to_string(i);
					ImGui::PushID(addID.c_str());

					ImGui::TableNextRow();
					ImGui::TableNextColumn();

					ImGui::TextWrapped(node->GetName().c_str());
					ImGui::TableNextColumn();

					ImGui::Button("+", ImVec2(20, 20));
					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FileBrowser"))
						{
							std::string& path = *(std::string*)payload->Data;
							if (Tools::FileExtensionCheck(path, ".s_material"))
							{

								PBRCreateInfo pbrMat{};
								if (pbrMat.Load(path))
								{
									auto id = PBRFactory::AddMaterial(&pbrMat, path);
									comp->GetMeshView()->SetPBRHandle(id, i);
								}
							}
						}
						ImGui::EndDragDropTarget();
					}

					ImGui::SameLine();
					if (ImGui::Button("-", ImVec2(20, 20)))
					{
						comp->GetMeshView()->SetPBRHandle(nullptr, i);
					}

					ImGui::TableNextColumn();
					ImGui::PopID();
				}

				ImGui::EndTable();
				ImGui::NewLine();
			}
		}

		ImGui::SetCursorPosX(10);
		ImGui::PushID("MeshID");
		if (ImGui::Button("Load", { ImGui::GetWindowWidth() - 25.0f, 30.0f }))
		{
			const auto& result = Utils::OpenFile("glTF 2.0 (*gltf)\0*.gltf\0");
			if (result.has_value())
			{
				comp->LoadAnimation(result.value());
			}
		}
		ImGui::PopID();

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FileBrowser"))
			{
				std::string& path = *(std::string*)payload->Data;
				if (Tools::FileExtensionCheck(path, ".gltf"))
				{
					comp->LoadAnimation(path);
				}

			}
			ImGui::EndDragDropTarget();
		}

		ImGui::NewLine();
	}

	void EditorLayer::DrawPointLightComponent(PointLightComponent* comp)
	{
		bool* is_active = (bool*)&comp->IsActive;

		ImGui::Extensions::CheckBox("Enabled", *is_active);
		ImGui::Extensions::DragFloat("Exposure", comp->Intensity);
		ImGui::Extensions::DragFloat("Radius", comp->Raduis);
		ImGui::Extensions::ColorInput4("Color", comp->Color);
	}

	void EditorLayer::DrawSpotLightComponent(SpotLightComponent* light)
	{
		bool* is_active = (bool*)&light->IsActive;
		glm::vec3* dir = (glm::vec3*)&light->Direction;

		ImGui::Extensions::CheckBox("Enabled", *is_active);
		ImGui::Extensions::InputFloat3("Direction", *dir);
		ImGui::Extensions::DragFloat("Exposure", light->Intensity);
		ImGui::Extensions::DragFloat("CutOff", light->CutOff);
		ImGui::Extensions::DragFloat("Outer CutOff", light->OuterCutOff);
		ImGui::Extensions::DragFloat("Bias", light->Bias);
		ImGui::Extensions::DragFloat("Radius", light->Raduis);
		ImGui::Extensions::ColorInput4("Color", light->Color);
	}

	void EditorLayer::DrawRigidBodyComponent(RigidbodyComponent* component)
	{
		ImGui::Extensions::Combo("Type", "Dynamic\0Static\0Kinematic\0", component->CreateInfo.StateIndex);
		ImGui::Extensions::Combo("Shape", "Box\0Sphere\0Capsule\0Mesh\0", component->CreateInfo.ShapeIndex);

		component->CreateInfo.eType = (RigidBodyType)component->CreateInfo.StateIndex;
		component->CreateInfo.eShape = (RigidBodyShape)component->CreateInfo.ShapeIndex;

		if (component->CreateInfo.eShape == RigidBodyShape::Box)
		{
			ImGui::Extensions::DragFloat("X", component->CreateInfo.Size.x);
			ImGui::Extensions::DragFloat("Y", component->CreateInfo.Size.y);
			ImGui::Extensions::DragFloat("Z", component->CreateInfo.Size.z);
		}

		if (component->CreateInfo.eShape == RigidBodyShape::Sphere)
		{
			ImGui::Extensions::InputFloat("Radius", component->CreateInfo.Size.x);
		}

		if (component->CreateInfo.eShape == RigidBodyShape::Capsule)
		{
			ImGui::Extensions::DragFloat("Radius", component->CreateInfo.Size.x);
			ImGui::Extensions::DragFloat("Height", component->CreateInfo.Size.y);
		}

		if (component->CreateInfo.eShape == RigidBodyShape::Convex)
		{
			ImGui::NewLine();
			ImGui::Separator();
			ImGui::NewLine();

			if (!component->CreateInfo.FilePath.empty())
			{
				std::filesystem::path p(component->CreateInfo.FilePath);
				ImGui::SetCursorPosX(12);
				ImGui::Extensions::Text(p.filename().stem().u8string(), "");
			}

			ImGui::SetCursorPosX(10);
			if (ImGui::Button("Select Mesh", { ImGui::GetWindowWidth() - 20.0f, 20.0f }))
			{
				const auto& result = Utils::OpenFile("glTF 2.0 (*gltf)\0*.gltf\0");
				if (result.has_value())
					component->CreateInfo.FilePath = result.value();
			}

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FileBrowser"))
				{
					std::string& path = *(std::string*)payload->Data;
					if (Tools::FileExtensionCheck(path, ".gltf"))
					{
						component->CreateInfo.FilePath = path;
					}
				}
				ImGui::EndDragDropTarget();
			}

			ImGui::SetCursorPosX(10);
			if (ImGui::Button("Clear", { ImGui::GetWindowWidth() - 20.0f, 20.0f }))
			{
				component->CreateInfo.FilePath = "";
			}

			ImGui::NewLine();
		}

		ImGui::Separator();
		ImGui::NewLine();

		if (component->CreateInfo.eType == RigidBodyType::Dynamic)
		{
			ImGui::Extensions::DragFloat("Mass", component->CreateInfo.Mass);
		}

		ImGui::Extensions::DragFloat("Friction", component->CreateInfo.Friction, 0.5f);
		ImGui::Extensions::DragFloat("Restitution", component->CreateInfo.Restitution, 0.5f);
		ImGui::Extensions::DragFloat("Linear Damping", component->CreateInfo.LinearDamping, 0.5f);
		ImGui::Extensions::DragFloat("Angular Damping", component->CreateInfo.AngularDamping, 0.5f);
		ImGui::Extensions::DragFloat("Rolling Friction", component->CreateInfo.RollingFriction, 0.5f);
		ImGui::Extensions::DragFloat("Spinning Friction", component->CreateInfo.SpinningFriction, 0.5f);

		ImGui::NewLine();
	}

	void EditorLayer::DrawPostProcessingComponent(PostProcessingComponent* component)
	{
		if (ImGui::BeginTabBar("EnvironmentTab"))
		{
			if (ImGui::BeginTabItem("Bloom"))
			{
				auto& bloom = component->Bloom;

				ImGui::NewLine();
				ImGui::PushID("EnvironmentTabBloomID");

				if (ImGui::Extensions::CheckBox("Enabled", component->Bloom.Enabled))
					RendererStorage::GetState().Bloom = component->Bloom;

				if (ImGui::Extensions::DragFloat("Threshold", bloom.Threshold))
					RendererStorage::GetState().Bloom = component->Bloom;

				if (ImGui::Extensions::DragFloat("Exposure", bloom.Exposure))
					RendererStorage::GetState().Bloom = component->Bloom;

				if (ImGui::Extensions::DragFloat("Knee", bloom.Knee))
					RendererStorage::GetState().Bloom = component->Bloom;

				if (ImGui::Extensions::DragFloat("SkyboxMod", bloom.SkyboxMod))
					RendererStorage::GetState().Bloom = component->Bloom;


				ImGui::PopID();
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("FXAA"))
			{
				auto& fxaa = component->FXAA;

				ImGui::NewLine();
				ImGui::PushID("EnvironmentTabFXAAD");

				if (ImGui::Extensions::CheckBox("Enabled", component->FXAA.Enabled))
					RendererStorage::GetState().FXAA = component->FXAA;

				if (ImGui::Extensions::InputFloat("Threshold Max", fxaa.EdgeThresholdMax))
					RendererStorage::GetState().FXAA = component->FXAA;

				if (ImGui::Extensions::InputFloat("Threshold Min", fxaa.EdgeThresholdMin))
					RendererStorage::GetState().FXAA = component->FXAA;

				if (ImGui::Extensions::InputFloat("Iterations", fxaa.Iterations))
					RendererStorage::GetState().FXAA = component->FXAA;

				if (ImGui::Extensions::InputFloat("SubPixelQuality", fxaa.SubPixelQuality))
					RendererStorage::GetState().FXAA = component->FXAA;

				ImGui::PopID();
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}

		ImGui::NewLine();
	}

	void EditorLayer::DrawSkyLightComponent(SkyLightComponent* component)
	{
		if (ImGui::BeginTabBar("SkyLightTab"))
		{
			if (ImGui::BeginTabItem("Sky"))
			{
				ImGui::NewLine();
				ImGui::PushID("SkyLightTabSkyID");

				ImGui::Extensions::Combo("Type", "Dynamic\0Static\0", component->EnvironmentFlags);
				if (component->EnvironmentFlags == 0)
				{
					ImGui::NewLine();

					glm::vec3* sunPos = (glm::vec3*)(&component->SkyProperties.SunPosition);
					glm::vec3* rayOrigin = (glm::vec3*)(&component->SkyProperties.RayOrigin);

					ImGui::Extensions::CheckBox("PBR", component->bGeneratePBRMaps);
					{
						ImGui::SameLine();
						ImGui::TextDisabled("(?)");
						if (ImGui::IsItemHovered())
						{
							ImGui::BeginTooltip();
							ImGui::PushTextWrapPos(450.0f);
							ImGui::TextUnformatted("Generate BRDFLUT, Irradiance and Prefiltered maps.");
							ImGui::PopTextWrapPos();
							ImGui::EndTooltip();
						}
					}

					ImGui::Extensions::DragFloat3("Sun Position", *sunPos);
					ImGui::Extensions::DragFloat3("Ray Origin", *rayOrigin);
					ImGui::Extensions::DragFloat("Sun Intensity", component->SkyProperties.SunIntensity);
					ImGui::Extensions::DragFloat("Planet Radius", component->SkyProperties.PlanetRadius);
					ImGui::Extensions::DragFloat("Atm Radius", component->SkyProperties.AtmosphereRadius);
					ImGui::Extensions::DragFloat("Mie Direction", component->SkyProperties.MieScatteringDirection);


					int* numCirrus = (int*)(&component->SkyProperties.NumCirrusCloudsIterations);
					int* numCumulus = (int*)(&component->SkyProperties.NumCumulusCloudsIterations);
					ImGui::Extensions::InputInt("Cirrus Clouds", *numCirrus);
					ImGui::Extensions::InputInt("Cumulus Clouds", *numCumulus);

					ImGui::NewLine();
					ImGui::SetCursorPosX(6.0f);
					if (ImGui::Button("Update"))
					{
						auto frostium = GraphicsEngineSComponent::Get();

						RendererStorage::SetDynamicSkybox(component->SkyProperties,
							frostium->ViewProj.Projection, component->bGeneratePBRMaps);
					}

					ImGui::SameLine();
					if (ImGui::Button("Reset"))
					{
						component->SkyProperties = {};
						auto frostium = GraphicsEngineSComponent::Get();

						RendererStorage::SetDynamicSkybox(component->SkyProperties,
							frostium->ViewProj.Projection, component->bGeneratePBRMaps);
					}
				}
				else
				{
					ImGui::Separator();
					ImGui::NewLine();

					ImGui::Image(TexturesLoader::Get()->m_BackgroundIcon->GetImGuiTexture(), ImVec2{ 60, 60 }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FileBrowser"))
						{
							std::string& path = *(std::string*)payload->Data;
							if (Tools::FileExtensionCheck(path, ".ktx"))
							{
								TextureCreateInfo texCI = {};
								if (texCI.Load(path) == true)
								{
									component->CubeMap = Texture::Create();
									component->CubeMap->LoadAsCubeFromKtx(&texCI);

									RendererStorage::SetStaticSkybox(component->CubeMap);
								}
							}
						}
						ImGui::EndDragDropTarget();
					}
					ImGui::SameLine();
					ImGui::TextDisabled("(?)");
					if (ImGui::IsItemHovered())
					{
						ImGui::BeginTooltip();
						ImGui::PushTextWrapPos(450.0f);
						ImGui::TextUnformatted("only .ktx textures are supported");
						ImGui::PopTextWrapPos();
						ImGui::EndTooltip();
					}
				}

				ImGui::PopID();
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Light"))
			{
				ImGui::NewLine();
				ImGui::PushID("SkyLightTabLightID");

				if (ImGui::Extensions::CheckBox("Enable", component->IBLProperties.Enabled))
				{
					RendererStorage::GetState().IBL = component->IBLProperties;
				}

				if (ImGui::Extensions::DragFloat("Strength", component->IBLProperties.IBLStrength))
				{
					RendererStorage::GetState().IBL = component->IBLProperties;
				}

				if (ImGui::Extensions::ColorInput4("Color", component->IBLProperties.AmbientColor))
				{
					RendererStorage::GetState().IBL = component->IBLProperties;
				}

				ImGui::PopID();
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}
	
		ImGui::NewLine();
	}

	void EditorLayer::DrawDirectionalLightComponent(DirectionalLightComponent* component)
	{
		glm::vec3* dir = (glm::vec3*) & component->Direction;
		bool* is_active = (bool*)&component->IsActive;

		if (ImGui::Extensions::CheckBox("Enable", *is_active))
		{
			RendererDrawList::SubmitDirLight(dynamic_cast<DirectionalLight*>(component));
		}

		if (ImGui::Extensions::Combo("Shadows", "None\0Hard\0Soft\0", component->ShadowsFlags))
		{
			ShadowType type = (ShadowType)component->ShadowsFlags;
			switch (type)
			{
			case ShadowType::None: component->IsCastShadows = false; break;
			case ShadowType::Hard: component->IsCastShadows = true; component->IsUseSoftShadows = false; break;
			case ShadowType::Soft: component->IsCastShadows = true; component->IsUseSoftShadows = true; break;
			}

			RendererDrawList::SubmitDirLight(dynamic_cast<DirectionalLight*>(component));
		}

		if (ImGui::Extensions::DragFloat("Intensity", component->Intensity))
		{
			RendererDrawList::SubmitDirLight(dynamic_cast<DirectionalLight*>(component));
		}

		if (ImGui::Extensions::DragFloat3("Direction", *dir))
		{
			RendererDrawList::SubmitDirLight(dynamic_cast<DirectionalLight*>(component));
		}

		if (ImGui::Extensions::ColorInput4("Color", component->Color))
		{
			RendererDrawList::SubmitDirLight(dynamic_cast<DirectionalLight*>(component));
		}
	}

	void EditorLayer::DrawComponents()
	{
		if (ImGui::CollapsingHeader("Head"))
		{
			ImGui::NewLine();
			auto info = m_World->GetActiveScene()->GetComponent<HeadComponent>(m_SelectedActor);
			DrawInfo(info);
		}

		if (ImGui::CollapsingHeader("Tranform"))
		{
			ImGui::NewLine();
			auto trans = m_World->GetActiveScene()->GetComponent<TransformComponent>(m_SelectedActor);
			ImGui::Extensions::TransformComponent(trans->WorldPos, trans->Scale, trans->Rotation);
		}

		for (uint32_t i = 0; i < m_SelectedActor->GetComponentsCount(); ++i)
		{
			if (IsCurrentComponent<Texture2DComponent>(i))
			{
				if (ImGui::CollapsingHeader("Texture 2D"))
				{
					ImGui::NewLine();
					auto component = m_World->GetActiveScene()->GetComponent<Texture2DComponent>(m_SelectedActor);
					DrawTexture(component);
				}
			}

			if (IsCurrentComponent<Rigidbody2DComponent>(i))
			{
				if (ImGui::CollapsingHeader("Rigidbody 2D"))
				{
					ImGui::NewLine();
					auto component = m_World->GetActiveScene()->GetComponent<Rigidbody2DComponent>(m_SelectedActor);
					DrawRigidBody2D(component);
				}
			}

			if (IsCurrentComponent<AudioSourceComponent>(i))
			{
				if (ImGui::CollapsingHeader("Audio Source"))
				{
					ImGui::NewLine();
					auto component = m_World->GetActiveScene()->GetComponent<AudioSourceComponent>(m_SelectedActor);
					DrawAudioSource(component);
				}
			}

			if (IsCurrentComponent<MeshComponent>(i))
			{
				if (ImGui::CollapsingHeader("Mesh"))
				{
					ImGui::NewLine();
					auto component = m_World->GetActiveScene()->GetComponent<MeshComponent>(m_SelectedActor);
					DrawMeshComponent(component);
				}
			}

			if (IsCurrentComponent<PointLightComponent>(i))
			{
				if (ImGui::CollapsingHeader("Point Light"))
				{
					ImGui::NewLine();
					auto component = m_World->GetActiveScene()->GetComponent<PointLightComponent>(m_SelectedActor);
					DrawPointLightComponent(component);
				}
			}

			if (IsCurrentComponent<SpotLightComponent>(i))
			{
				if (ImGui::CollapsingHeader("Spot Light"))
				{
					ImGui::NewLine();
					auto component = m_World->GetActiveScene()->GetComponent<SpotLightComponent>(m_SelectedActor);
					DrawSpotLightComponent(component);
				}
			}

			if (IsCurrentComponent<SkyLightComponent>(i))
			{
				if (ImGui::CollapsingHeader("Sky Light"))
				{
					ImGui::NewLine();
					auto component = m_World->GetActiveScene()->GetComponent<SkyLightComponent>(m_SelectedActor);
					DrawSkyLightComponent(component);
				}
			}

			if (IsCurrentComponent<DirectionalLightComponent>(i))
			{
				if (ImGui::CollapsingHeader("Directional Light"))
				{
					ImGui::NewLine();
					auto component = m_World->GetActiveScene()->GetComponent<DirectionalLightComponent>(m_SelectedActor);
					DrawDirectionalLightComponent(component);
				}
			}

			if (IsCurrentComponent<CameraComponent>(i))
			{
				if (ImGui::CollapsingHeader("Camera"))
				{
					ImGui::NewLine();
					auto component = m_World->GetActiveScene()->GetComponent<CameraComponent>(m_SelectedActor);
					DrawCamera(component);
				}
			}

			if (IsCurrentComponent<RigidbodyComponent>(i))
			{
				if (ImGui::CollapsingHeader("Rigidbody"))
				{
					ImGui::NewLine();
					auto component = m_World->GetActiveScene()->GetComponent<RigidbodyComponent>(m_SelectedActor);
					DrawRigidBodyComponent(component);
				}
			}

			if (IsCurrentComponent<PostProcessingComponent>(i))
			{
				if (ImGui::CollapsingHeader("Post Processing"))
				{
					ImGui::NewLine();
					auto component = m_World->GetActiveScene()->GetComponent<PostProcessingComponent>(m_SelectedActor);
					DrawPostProcessingComponent(component);
				}
			}

			DrawScriptComponent(i);
		}

		ImGui::Separator();
		ImGui::NewLine();
	}

	void EditorLayer::DrawComponentPopUp()
	{
		if (ImGui::BeginPopup("AddComponentPopUp"))
		{
			ImGui::MenuItem("#General", NULL, false, false);
			{
				ImGui::Separator();

				if (ImGui::MenuItem("Mesh"))
				{
					m_World->GetActiveScene()->AddComponent<MeshComponent>(m_SelectedActor);
					ImGui::CloseCurrentPopup();
				}

				if (ImGui::MenuItem("Texture"))
				{
					m_World->GetActiveScene()->AddComponent<Texture2DComponent>(m_SelectedActor);
					ImGui::CloseCurrentPopup();
				}

				if (ImGui::MenuItem("Camera"))
				{
					m_World->GetActiveScene()->AddComponent<CameraComponent>(m_SelectedActor);
					ImGui::CloseCurrentPopup();
				}

				if (ImGui::MenuItem("Canvas"))
				{
					m_World->GetActiveScene()->AddComponent<CanvasComponent>(m_SelectedActor);
					ImGui::CloseCurrentPopup();
				}
			}

			ImGui::MenuItem("#Audio", NULL, false, false);
			{
				ImGui::Separator();
				if (ImGui::MenuItem("Audio Source"))
				{
					m_World->GetActiveScene()->AddComponent<AudioSourceComponent>(m_SelectedActor);
					ImGui::CloseCurrentPopup();
				}
			}

			ImGui::MenuItem("#Renderer", NULL, false, false);
			{
				ImGui::Separator();

				if (ImGui::MenuItem("Post Processing"))
				{
					m_World->GetActiveScene()->AddComponent<PostProcessingComponent>(m_SelectedActor);
					ImGui::CloseCurrentPopup();
				}
			}

			ImGui::MenuItem("#Lighting", NULL, false, false);
			{
				ImGui::Separator();

				if (ImGui::MenuItem("Spot Light"))
				{
					m_World->GetActiveScene()->AddComponent<SpotLightComponent>(m_SelectedActor);
					ImGui::CloseCurrentPopup();
				}

				if (ImGui::MenuItem("Point Light"))
				{
					m_World->GetActiveScene()->AddComponent<PointLightComponent>(m_SelectedActor);
					ImGui::CloseCurrentPopup();
				}

				if (ImGui::MenuItem("Sky Light"))
				{
					m_World->GetActiveScene()->AddComponent<SkyLightComponent>(m_SelectedActor);
					ImGui::CloseCurrentPopup();
				}

				if (ImGui::MenuItem("Directional Light"))
				{
					m_World->GetActiveScene()->AddComponent<DirectionalLightComponent>(m_SelectedActor);
					ImGui::CloseCurrentPopup();
				}
			}

			ImGui::MenuItem("#Physics", NULL, false, false);
			{
				ImGui::Separator();

				if (ImGui::MenuItem("RigidBody"))
				{
					auto comp = m_World->GetActiveScene()->AddComponent<RigidbodyComponent>(m_SelectedActor);
					comp->Validate(m_SelectedActor);
					ImGui::CloseCurrentPopup();
				}

				if (ImGui::MenuItem("2D RigidBody"))
				{
					//auto comp = m_World->GetActiveScene()->AddComponent<Rigidbody2DComponent>(m_SelectedActor);
					//ComponentHandler::ValidateBody2DComponent(comp, m_SelectedActor);

					ImGui::CloseCurrentPopup();
				}
			}

			ImGui::EndPopup();
		}
	}

	void EditorLayer::DrawScriptPopUp()
	{
		if (ImGui::BeginPopup("AddCScriptPopUp"))
		{
			ImGui::MenuItem("New Script", NULL, false, false);
			ImGui::Separator();

			auto& meta_map = ScriptingSystemStateSComponent::GetSingleton()->m_MetaContext->GetMeta();

			for (const auto& [name, meta] : meta_map)
			{
				if (ImGui::MenuItem(name.c_str()))
				{
					ScriptingSystem::AttachNativeScript(m_SelectedActor, name);
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::EndPopup();
		}

		if (ImGui::BeginPopup("AddCSharpScriptPopUp"))
		{
			ImGui::MenuItem("New Script", NULL, false, false);
			ImGui::Separator();
			auto& meta_map = ScriptingSystemStateSComponent::GetSingleton()->m_MonoContext->m_MetaMap;
			for (const auto& [name, meta] : meta_map)
			{
				if (ImGui::MenuItem(name.c_str()))
				{
					ScriptingSystem::AttachCSharpScript(m_SelectedActor, name);
					ImGui::CloseCurrentPopup();
				}
			}

			ImGui::EndPopup();
		}
	}

	void EditorLayer::CheckSelectedActor()
	{
		if (m_SelectedActor != nullptr)
		{
			if (!m_World->GetActiveScene()->GetRegistry().valid(*m_SelectedActor))
			{
				m_SelectedActor = nullptr;
			}
		}
	}

	void EditorLayer::ResetSelection()
	{
		m_TempActorTag = "";
		m_TempActorName = "";
		m_SelectedActor = nullptr;
		m_SelectionFlags = SelectionFlags::None;
		m_FileExplorer->ClearSelection();
	}

	void EditorLayer::DrawScriptComponent(uint32_t index)
	{
		Scene* scene = m_World->GetActiveScene();

		if (scene->HasComponent<ScriptComponent>(m_SelectedActor))
		{
			ScriptComponent* comp = scene->GetComponent<ScriptComponent>(m_SelectedActor);
			if (comp->ComponentID != index)
				return;

			constexpr auto DrawFieldsFunc = [](FieldManager& filedManager)
			{
				for (auto& field : filedManager.GetFields())
				{
					if (field.type == FieldDataFlags::Int32)
					{
						ImGui::Extensions::DragInt(field.name, *(int32_t*)field.ptr);
					}

					if (field.type == FieldDataFlags::Float)
					{
						ImGui::Extensions::DragFloat(field.name, *(float*)field.ptr);
					}

					if (field.type == FieldDataFlags::String)
					{
						ImGui::Extensions::InputRawString(field.name, *(std::string*)field.ptr);
					}

					if (field.type == FieldDataFlags::Actor)
					{
						auto& value = *(int32_t*)field.ptr;
						if (field.description.empty() && value > 0)
						{
							Ref<Actor> target = WorldAdmin::GetSingleton()->GetActiveScene()->FindActorByID(value);
							if (target)
							{
								auto& desc = const_cast<std::string&>(field.description);
								desc = target->GetName();
							}
							else
							{
								value = 0;
							}
						}

						ImGui::Extensions::InputRawString(field.name, const_cast<std::string&>(field.description), "", ImGuiInputTextFlags_ReadOnly);
						if (ImGui::BeginDragDropTarget())
						{
							if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ActorDragAndDrop"))
							{
								uint32_t id = *static_cast<uint32_t*>(payload->Data);
								Ref<Actor> target = WorldAdmin::GetSingleton()->GetActiveScene()->FindActorByID(id);
								if (target)
								{
									auto& desc = const_cast<std::string&>(field.description);
									desc = target->GetName();
									value = static_cast<int32_t>(target->GetID());
								}
							}

							ImGui::EndDragDropTarget();
						}
					}

					if (field.type == FieldDataFlags::Prefab)
					{
						std::filesystem::path p(*(std::string*)field.ptr);
						ImGui::Extensions::InputRawString(field.name, p.filename().stem().u8string(), "", ImGuiInputTextFlags_ReadOnly);
						if (ImGui::BeginDragDropTarget())
						{
							if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FileBrowser"))
							{
								std::string& path = *(std::string*)payload->Data;
								if (Tools::FileExtensionCheck(path, ".s_prefab"))
								{
									*(std::string*)field.ptr = path;
								}
							}
							ImGui::EndDragDropTarget();
						}
					}
				}
			};

			for (auto& script : comp->CppScripts)
			{
				std::string title = script.Name + " (C++)";
				if (ImGui::CollapsingHeader(title.c_str()))
				{
					ImGui::NewLine();
					DrawFieldsFunc(script.Fields);
					ImGui::NewLine();
				}
			}

			for (auto& script : comp->CSharpScripts)
			{
				std::string title = script.Name + " (C#)";
				if (ImGui::CollapsingHeader(title.c_str()))
				{
					ImGui::NewLine();
					DrawFieldsFunc(script.Fields);
					ImGui::NewLine();
				}
			}
		}
	}

	void EditorLayer::DrawMeshPrimitive(uint32_t type, const std::string& title, const std::string& desc, Ref<Texture>& icon)
	{
		ImGui::TableNextRow();
		ImGui::TableNextColumn();

		ImGui::TextUnformatted(title.c_str());
		ImGui::TableNextColumn();

		ImGui::Image(icon->GetImGuiTexture(), { 60, 60 }, ImVec2(0, 1), ImVec2(1, 0));
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
		{
			m_IDBuffer = type;
			ImGui::SetDragDropPayload("MeshPanel", &m_IDBuffer, sizeof(uint32_t));

			ImGui::Image(icon->GetImGuiTexture(), { 40, 40 }, ImVec2(0, 1), ImVec2(1, 0));
			ImGui::EndDragDropSource();
		}

		ImGui::TableNextColumn();
		ImGui::TextWrapped(desc.c_str());

		ImGui::TableNextColumn();
	}

	void EditorLayer::CheckActor(Ref<Actor>& actor)
	{
		HeadComponent* head = actor->GetInfo();
		for (auto child : head->Childs)
		{
			if (std::find(m_DisplayedActors.begin(), m_DisplayedActors.end(), child) != m_DisplayedActors.end())
			{
				std::vector<Ref<Actor>> tmp = m_DisplayedActors;
				tmp.erase(std::remove(tmp.begin(), tmp.end(), child), tmp.end());
				m_DisplayedActors = tmp;

				CheckActor(child);
			}
		}
	}

	// TODO: remove hard coded strings
	void EditorLayer::OnFileSelected(const std::string& path, const std::string& ext, int fileSize)
	{
		m_AudioPanel->Reset();
		m_TextureInspector->Reset();
		m_MaterialInspector->Close();
		m_AnimationPanel->Reset();

		if (ext == ".s_material")
		{
			m_SelectedActor = nullptr;
			m_SelectionFlags = SelectionFlags::MaterialView;

			if (fileSize > 0)
				m_MaterialInspector->OpenExisting(path);
			else
				m_MaterialInspector->OpenNew(path);
		}

		if (ext == ".s_audio")
		{
			m_SelectedActor = nullptr;
			m_SelectionFlags = SelectionFlags::AuidoView;
			m_AudioPanel->Open(path);
		}

		if (ext == ".s_image")
		{
			m_SelectedActor = nullptr;
			m_SelectionFlags = SelectionFlags::TextureView;
			m_TextureInspector->Open(path);
		}

		if (ext == ".s_animation")
		{
			m_SelectedActor = nullptr;
			m_SelectionFlags = SelectionFlags::AnimationView;
			m_AnimationPanel->Open(path, fileSize == 0);
		}
	}

	void EditorLayer::OnFileDeleted(const std::string& path, const std::string& ext)
	{
		if (ext == ".s_material")
		{
			m_MaterialInspector->Close();
		}

		m_SelectionFlags = SelectionFlags::None;
	}
}