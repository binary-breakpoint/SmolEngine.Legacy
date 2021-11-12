#include "stdafx.h"
#include "ECS/Systems/RendererSystem.h"
#include "ECS/Components/Singletons/WorldAdminStateSComponent.h"
#include "ECS/Components/Singletons/Bullet3WorldSComponent.h"
#include "ECS/Components/Singletons/GraphicsEngineSComponent.h"
#include "ECS/ComponentsCore.h"
#include "ECS/Systems/UISystem.h"

#include <btBulletDynamicsCommon.h>

namespace SmolEngine
{
	void RendererSystem::OnUpdate()
	{
		// 3D
		RendererDrawList& drawList = m_State->DrawList;
		drawList.BeginSubmit(&m_State->ViewProj);
		{
			SubmitMeshes();
			SubmitLights();
		}
		drawList.EndSubmit();

		// 2D
		RendererDrawList2D& drawList2D = m_State->DrawList2D;
		drawList2D.BeginSubmit(&m_State->ViewProj);
		{
			SubmitSprites();
		}
		drawList2D.EndSubmit();
	}

	void RendererSystem::OnRender()
	{
		OnUpdate();
		ClearInfo clear = {};
		clear.bClear = true;
		RendererDeferred::DrawFrame(&clear, &m_State->Storage, &m_State->DrawList);

		clear.bClear = false;
		Renderer2D::DrawFrame(&clear, &m_State->Storage2D, &m_State->DrawList2D);

		OnDebugDraw();
	}

	int RendererSystem::GetLayerIndex(int index)
	{
		if (index > 10) { return 10; }
		if (index < 0) { return 0; }

		return index;
	}

	void RendererSystem::OnDebugDraw()
	{
		if (m_State->eDebugDrawFlags == DebugDrawFlags::Disabled)
			return;

		entt::registry* reg = m_World->m_CurrentRegistry;

		RendererDebug::BeginDebug();
		{
			switch (m_State->eDebugDrawFlags)
			{
			case DebugDrawFlags::Bullet: Bullet3WorldSComponent::Get()->World->debugDrawWorld(); break;
			case DebugDrawFlags::Default:
			{
				const auto& dynamic_group = m_World->m_CurrentRegistry->view<TransformComponent, RigidbodyComponent>();
				for (const auto& entity : dynamic_group)
				{
					const auto& [transform, rigidbodyComponent] = dynamic_group.get<TransformComponent, RigidbodyComponent>(entity);
					BodyCreateInfo& info = rigidbodyComponent.CreateInfo;

					switch (info.eShape)
					{
					case RigidBodyShape::Box:
					{
						RendererDebug::DrawBox(glm::vec3(-1, -1, -1), glm::vec3(1, 1, 1), transform.WorldPos, transform.Rotation, info.Size);
						break;
					}
					case RigidBodyShape::Sphere:
					{
						RendererDebug::DrawSphere(info.Size.x, transform.WorldPos, transform.Rotation, glm::vec3(1.0f));
						break;
					}
					case RigidBodyShape::Capsule:
					{
						RendererDebug::DrawCapsule(info.Size.x, info.Size.y / 2.0f, 1, transform.WorldPos, transform.Rotation, glm::vec3(1.0f));
						break;
					}
					}
				}

				break;
			}
			case DebugDrawFlags::Wireframes:
			{
				const auto& group = reg->view<TransformComponent, MeshComponent>();
				for (const auto& entity : group)
				{
					const auto& [transform, mesh_component] = group.get<TransformComponent, MeshComponent>(entity);
					if (mesh_component.bShow)
					{
						Mesh* mesh = mesh_component.RootMesh.get();
						if (mesh != nullptr)
							RendererDebug::DrawWireframes(transform.WorldPos, transform.Rotation, transform.Scale, mesh);
					}
				}
				break;
			}
			}
		}
		RendererDebug::EndDebug();

	}

	void RendererSystem::SubmitLights()
	{
		entt::registry* reg = m_World->m_CurrentRegistry;
		GraphicsEngineSComponent* engine = GraphicsEngineSComponent::Get();
		RendererDrawList& drawList = engine->DrawList;

		const auto& point_Group = reg->view<TransformComponent, PointLightComponent>();
		for (const auto& entity : point_Group)
		{
			const auto& [transform, comp] = point_Group.get<TransformComponent, PointLightComponent>(entity);

			if (comp.IsActive == 1)
			{
				comp.Position = glm::vec4(transform.WorldPos, 1.0);
				drawList.SubmitPointLight(dynamic_cast<PointLight*>(&comp));
			}
		}

		const auto& spot_Group = reg->view<TransformComponent, SpotLightComponent>();
		for (const auto& entity : spot_Group)
		{
			const auto& [transform, comp] = spot_Group.get<TransformComponent, SpotLightComponent>(entity);

			if (comp.IsActive == 1)
			{
				comp.Position = glm::vec4(transform.WorldPos, 1.0);
				drawList.SubmitSpotLight(dynamic_cast<SpotLight*>(&comp));
			}
		}
	}

	void RendererSystem::SubmitMeshes()
	{
		entt::registry* reg = m_World->m_CurrentRegistry;
		GraphicsEngineSComponent* engine = GraphicsEngineSComponent::Get();
		Scene* scene = WorldAdmin::GetSingleton()->GetActiveScene();
		RendererDrawList& drawList = engine->DrawList;

		const auto& group = reg->view<TransformComponent, MeshComponent>();
		for (const auto& entity : group)
		{
			const auto& [transform, mesh_component] = group.get<TransformComponent, MeshComponent>(entity);

			AnimationControllerComponent* anim_controller = scene->GetComponent< AnimationControllerComponent>(entity);
			AnimationController* controller = dynamic_cast<AnimationController*>(anim_controller);

			if (controller) { if (controller->GetActiveClip() == nullptr) { controller = nullptr; } }
			if (mesh_component.bShow == false || mesh_component.RootMesh == nullptr)
				continue;

			auto& info = mesh_component.Nodes;
			uint32_t count = static_cast<uint32_t>(info.size());
			for (uint32_t i = 0; i < count; i++)
			{
				drawList.SubmitMesh(transform.WorldPos, transform.Rotation, transform.Scale,
					info[i].Mesh, info[i].MaterialID, false, controller);
			}
		}
	}

	void RendererSystem::SubmitSprites()
	{
		entt::registry* reg = m_World->m_CurrentRegistry;
		GraphicsEngineSComponent* engine = GraphicsEngineSComponent::Get();
		RendererDrawList2D& drawList = engine->DrawList2D;

		const auto& group = reg->view<TransformComponent, Texture2DComponent>();
		for (const auto& entity : group)
		{
			const auto& [transform, texture2D] = group.get<TransformComponent, Texture2DComponent>(entity);
			texture2D.LayerIndex = GetLayerIndex(texture2D.LayerIndex);

			if (texture2D.Enabled && texture2D.Texture != nullptr)
			{
				drawList.SubmitSprite(transform.WorldPos,
					transform.Scale, transform.Rotation, texture2D.LayerIndex, texture2D.Texture.get(), true, texture2D.Color);
			}
		}
	}
}