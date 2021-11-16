#include "stdafx.h"
#include "Panels/ViewportPanel.h"
#include "EditorLayer.h"
#include "ImGuiExtension.h"
#include "Editor-Tools/Tools.h"

#include "ECS/Components/Singletons/GraphicsEngineSComponent.h"
#include "ECS/Components/CameraComponent.h"
#include "ECS/Systems/AudioSystem.h"
#include "ECS/Systems/RendererSystem.h"
#include "ECS/Prefab.h"

#include "Camera/EditorCamera.h"
#include "Tools/Utils.h"

#include <glm/glm/gtx/quaternion.hpp>
#include <glm/glm/gtx/matrix_decompose.hpp>
#include <glm/glm/gtc/type_ptr.hpp>

namespace SmolEngine
{
	ViewportPanel::ViewportPanel()
		:m_World(WorldAdmin::GetSingleton())
	{

	}

	SceneView::SceneView(EditorLayer* editor)
		:m_Editor(editor),
		m_ButtonSize(19.0f, 19.0f)
	{
		m_Camera = m_Editor->m_Camera;
		m_TexturesLoader = TexturesLoader::Get();
		m_GraphicsEngine = GraphicsEngineSComponent::Get();
	}

	void SceneView::OnUpdate(float delta)
	{
		if(IsFocused())
			m_Camera->OnUpdate(delta);
	}

	void SceneView::OnEvent(Event& e)
	{
		if (e.IsType(EventType::MOUSE_SCROLL) && IsFocused())
		{
			MouseScrollEvent* sroll = e.Cast<MouseScrollEvent>();
			m_Camera->OnMouseScroll(sroll->GetXoffset(), sroll->GetYoffset());
		}
	}

	void SceneView::Draw()
	{
		ImGui::Begin("Scene View", nullptr, ImGuiWindowFlags_NoDecoration);
		{
			Ref<Actor>& selectedActor = m_Editor->m_SelectedActor;
			DrawToolBar(selectedActor);

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0.0f, 0.0f });
			ImGui::BeginChild("TetxureScene");
			{
				DrawViewPort();
				DrawDragAndDrop(selectedActor);
				DrawGizmos(selectedActor);

			}
			ImGui::EndChild();
			ImGui::PopStyleVar();

		}
		ImGui::End();
	}

	void SceneView::DrawToolBar(Ref<Actor>& selectedActor)
	{
		ImGui::SetCursorPosX(10);
		ImGui::PushID("SceneView_ToolBar");

		if (ImGui::ImageButton(m_TexturesLoader->m_MoveButton->GetImGuiTexture(), m_ButtonSize)) { m_GizmoOperation = ImGuizmo::OPERATION::TRANSLATE; }
		ImGui::SameLine();
		if (ImGui::ImageButton(m_TexturesLoader->m_RotateButton->GetImGuiTexture(), m_ButtonSize)) { m_GizmoOperation = ImGuizmo::OPERATION::ROTATE; }
		ImGui::SameLine();
		if (ImGui::ImageButton(m_TexturesLoader->m_ScaleButton->GetImGuiTexture(), m_ButtonSize)) { m_GizmoOperation = ImGuizmo::OPERATION::SCALE; }

		ImGui::SameLine();
		ImGui::SetCursorPosX((ImGui::GetWindowWidth() / 2.0f) - 40);
		if (ImGui::ImageButton(m_TexturesLoader->m_PlayButton->GetImGuiTexture(), m_ButtonSize))
		{
			if (!m_World->IsInPlayMode())
			{
				m_World->SaveCurrentScene();
				m_World->OnBeginWorld();
			}
			else
			{
				DebugLog::LogWarn("The scene is already in play mode!");
			}
		}

		ImGui::SameLine();
		if (ImGui::ImageButton(m_TexturesLoader->m_StopButton->GetImGuiTexture(), m_ButtonSize))
		{
			if (m_World->IsInPlayMode())
			{
				uint32_t selectedActorID = 0;
				if (selectedActor != nullptr)
				{
					selectedActorID = selectedActor->GetID();
				}

				selectedActor = nullptr;
				m_World->OnEndWorld();
				m_World->LoadLastSceneState();
				selectedActor = m_World->GetActiveScene()->FindActorByID(selectedActorID);
			}
			else
			{
				DebugLog::LogWarn("The scene is not in play mode!");
			}
		}

		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetWindowWidth() / 25.0f));
		ImGui::PushItemWidth(120);
		{
			float coPos = ImGui::GetCursorPosY();
			ImGui::SetCursorPosY(coPos + 3);
			ImGui::TextUnformatted("Draw:");

			ImGui::SameLine();
			ImGui::SetCursorPosY(coPos);

			if (ImGui::Combo("##Draw Mode", &m_DrawModeComboIndex, "Default\0Albedo\0Position\0Normals\0Materials\0Emission\0ShadowMap\0ShadowMapCood\0AO\0"))
			{
				RendererStorage::GetState().eDebugView = (DebugViewFlags)m_DrawModeComboIndex;
			}

			ImGui::SameLine();
			ImGui::TextUnformatted("Debug:");
			ImGui::SameLine();
			if (ImGui::Combo("##Dubug Mode", &m_DebugComboIndex, "Disabled\0Default\0Bullet\0Wireframes\0"))
			{
				m_GraphicsEngine->eDebugDrawFlags = (DebugDrawFlags)m_DebugComboIndex;
			}

		}
		ImGui::PopItemWidth();

		ImGui::SameLine();
		ImGui::Checkbox("Gizmos", &m_GizmosEnabled);
		ImGui::SameLine();
		ImGui::Checkbox("Grid", &RendererStorage::GetState().bDrawGrid);

		ImGui::PopID();
	}

	void SceneView::DrawViewPort()
	{
		if (ImGui::IsWindowHovered()) { m_Focused = true; }
		else { m_Focused = false; }

		Ref<Framebuffer>& fb = GraphicsContext::GetSingleton()->GetMainFramebuffer();
		ImVec2 regionAvail = ImGui::GetContentRegionAvail();
		glm::ivec2 viewPortSize = { static_cast<uint32_t>(regionAvail.x), static_cast<uint32_t>(regionAvail.y) };

		if (viewPortSize.x != m_ViewPortSize.x || viewPortSize.y != m_ViewPortSize.y)
		{
			m_ViewPortSize = viewPortSize;
			m_Camera->OnResize(m_ViewPortSize.x, m_ViewPortSize.y);
			GraphicsContext::GetSingleton()->SetFramebufferSize(m_ViewPortSize.x, m_ViewPortSize.y);
		}

		ImGui::Image(fb->GetImGuiTextureID(), regionAvail);
	}

	void SceneView::DrawDragAndDrop(Ref<Actor>& selectedActor)
	{
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MeshPanel"))
			{
				m_Editor->m_SelectionFlags = SelectionFlags::Inspector;
				auto actor = m_World->GetActiveScene()->CreateActor();
				MeshComponent* component = actor->AddComponent<MeshComponent>();
				component->LoadMesh((MeshTypeEX) *(uint32_t*)payload->Data);

				m_Editor->m_SelectedActor = actor;
			}

			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FileBrowser"))
			{
				std::string path = *(std::string*)payload->Data;

				if (Tools::FileExtensionCheck(path, ".s_scene"))
				{
					bool reload = m_World->GetActiveScene()->GetSceneState()->FilePath == path;
					m_World->LoadScene(path, reload);
					selectedActor = nullptr;
				}

				if (Tools::FileExtensionCheck(path, ".s_prefab"))
				{
					selectedActor = nullptr;
					auto prefab = m_World->GetActiveScene()->LoadPrefab(path);
					if (prefab)
					{
						m_World->GetActiveScene()->InstantiatePrefab(prefab, glm::vec3(0));
					}
				}

				if (Tools::FileExtensionCheck(path, ".gltf"))
				{
					auto actor = m_World->GetActiveScene()->CreateActor();
					MeshComponent* mesh = actor->AddComponent<MeshComponent>();
					TransformComponent* transform = actor->GetComponent<TransformComponent>();

					{
						float rayDistance = 20.0f;
						float x = ImGui::GetMousePos().x;
						float y = ImGui::GetMousePos().y - (m_ViewPortSize.y / 2.0f);

						glm::vec3 startPos = m_Editor->m_Camera->GetPosition();
						glm::mat4 viewProj = m_Editor->m_Camera->GetViewProjection();

						transform->WorldPos = Utils::CastRay(startPos, glm::vec2(x, y), static_cast<float>(m_ViewPortSize.x), static_cast<float>(m_ViewPortSize.y), rayDistance, viewProj);
					}

					if (mesh)
					{
						mesh->LoadMesh(path);
						selectedActor = actor;
					}
					else
					{
						selectedActor = nullptr;
					}
				}
			}
			ImGui::EndDragDropTarget();
		}
	}

	void SceneView::DrawGizmos(Ref<Actor>& selectedActor)
	{
		// Gizmos
		if (selectedActor != nullptr && m_GizmosEnabled)
		{
			auto transformComponent = selectedActor->GetComponent<TransformComponent>();
			if (transformComponent)
			{
				float snapValue = 0.5f;
				switch (m_Editor->m_Camera->GetType())
				{
				case CameraType::Perspective:
				{
					ImGuizmo::SetOrthographic(false);
					break;
				}
				case CameraType::Ortho:
				{
					ImGuizmo::SetOrthographic(true);
					break;
				}
				default:
					break;
				}

				if (m_GizmoOperation == ImGuizmo::OPERATION::ROTATE)
					snapValue = 45.0f;

				ImGuizmo::SetDrawlist();
				float width = (float)ImGui::GetWindowSize().x;
				float height = (float)ImGui::GetWindowSize().y;
				ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, width, height);

				glm::mat4 transform;
				glm::mat4 rotation = glm::toMat4(glm::quat(transformComponent->Rotation));
				transform = glm::translate(glm::mat4(1.0f), transformComponent->WorldPos)
					* rotation
					* glm::scale(glm::mat4(1.0f), transformComponent->Scale);

				float snapValues[3] = { snapValue, snapValue, snapValue };

				ImGuizmo::Manipulate(glm::value_ptr(m_Editor->m_Camera->GetViewMatrix()), glm::value_ptr(m_Editor->m_Camera->GetProjection()),
					m_GizmoOperation, ImGuizmo::LOCAL, glm::value_ptr(transform), nullptr, m_SnapEnabled ? snapValues : nullptr);

				if (ImGuizmo::IsUsing())
				{
					glm::vec3 tranlation, rotation, scale;
					Utils::DecomposeTransform(transform, tranlation, rotation, scale);

					selectedActor->SetPosition(tranlation);
					selectedActor->SetRotation(rotation);
					selectedActor->SetScale(scale);
				}
			}
		}
	}

	GameView::GameView()
	{
		FramebufferSpecification framebufferCI = {};
		const WindowData* windowData = GraphicsContext::GetSingleton()->GetWindow()->GetWindowData();

		framebufferCI.Width = windowData->Width;
		framebufferCI.Height = windowData->Width;
		framebufferCI.eMSAASampels = MSAASamples::SAMPLE_COUNT_1;
		framebufferCI.bTargetsSwapchain = false;
		framebufferCI.bResizable = true;
		framebufferCI.bAutoSync = false;
		framebufferCI.bUsedByImGui = true;
		framebufferCI.Attachments = { FramebufferAttachment(AttachmentFormat::Color) };

		m_PreviewFramebuffer = Framebuffer::Create();
		m_PreviewFramebuffer->Build(&framebufferCI);
	}

	void GameView::Draw()
	{
		if (m_Active)
		{
			ImGui::Begin("Game View", &m_Active);
			{
				if (ImGui::IsWindowHovered()) { m_Focused = true; }
				else { m_Focused = false; }

				Ref<Framebuffer>& fb = m_PreviewFramebuffer;
				ImVec2 regionAvail = ImGui::GetContentRegionAvail();
				glm::ivec2 viewPortSize = { static_cast<uint32_t>(regionAvail.x), static_cast<uint32_t>(regionAvail.y) };

				if (viewPortSize.x != m_ViewPortSize.x || viewPortSize.y != m_ViewPortSize.y)
				{
					m_ViewPortSize = viewPortSize;
					m_PreviewFramebuffer->OnResize(static_cast<uint32_t>(m_ViewPortSize.x), static_cast<uint32_t>(m_ViewPortSize.y));
				}

				ImGui::Image(fb->GetImGuiTextureID(), regionAvail);
			}

			ImGui::End();
		}

		if (!m_Active && m_Focused)
			m_Focused = false;
	}

	void GameView::Render()
	{
		auto& reg = m_World->GetActiveScene()->GetRegistry();
		const auto& cameraGroup = reg.view<CameraComponent, TransformComponent>();
		for (const auto& entity : cameraGroup)
		{
			const auto& [camera, transform] = cameraGroup.get<CameraComponent, TransformComponent>(entity);
			if (camera.bPrimaryCamera == false) { continue; }

			// Calculating ViewProj
			camera.CalculateView(&transform);

			GraphicsEngineSComponent* engine = GraphicsEngineSComponent::Get();

			SceneViewProjection sceneInfo = engine->ViewProj;

			engine->ViewProj.View = camera.ViewMatrix;
			engine->ViewProj.Projection = camera.ProjectionMatrix;
			engine->ViewProj.NearClip = camera.zNear;
			engine->ViewProj.FarClip = camera.zFar;
			engine->ViewProj.CamPos = glm::vec4(transform.WorldPos, 1);
			engine->ViewProj.SkyBoxMatrix = glm::mat4(glm::mat3(camera.ViewMatrix));

			AudioSystem::OnUpdate(transform.WorldPos);

			RendererDrawList::CalculateFrustum(&engine->ViewProj);
			RendererStorage::SetRenderTarget(m_PreviewFramebuffer);
			
			const bool batch_cmd = false;
			ClearInfo clearInfo;
			RendererSystem::OnUpdate();
			RendererDeferred::DrawFrame(&clearInfo, batch_cmd);

			engine->ViewProj = sceneInfo;
			RendererStorage::SetRenderTarget(GraphicsContext::GetSingleton()->GetMainFramebuffer());
			RendererDrawList::CalculateFrustum(&sceneInfo);
			break;
		}
	}
}