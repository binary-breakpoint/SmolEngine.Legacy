#include "stdafx.h"
#include "ECS/Systems/RendererSystem.h"
#include "ECS/Components/Include/Components.h"
#include "ECS/Components/Singletons/WorldAdminStateSComponent.h"
#include "ECS/Components/Singletons/Bullet3WorldSComponent.h"
#include "ECS/Components/Singletons/GraphicsEngineSComponent.h"
#include "ECS/Systems/UISystem.h"

#include "Materials/PBRFactory.h"

#include <btBulletDynamicsCommon.h>

namespace SmolEngine
{
	void RendererSystem::OnUpdate()
	{
		auto viewProj = &GraphicsEngineSComponent::Get()->ViewProj;

		RendererDrawList::BeginSubmit(viewProj);
		{
			SubmitLights();
			SubmitMeshes();
		}
		RendererDrawList::EndSubmit();

		RendererDrawList2D::BeginSubmit(viewProj);
		{
			SubmitSprites();
		}
		RendererDrawList2D::EndSubmit();
	}

	void RendererSystem::OnRender()
	{
		OnUpdate();

		ClearInfo clear = {};
		clear.bClear = true;
		RendererDeferred::DrawFrame(&clear);

		clear.bClear = false;
		Renderer2D::DrawFrame(&clear);

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
						//if (mesh_component.GetMesh()!= nullptr)
						//	RendererDebug::DrawWireframes(transform.WorldPos, transform.Rotation, transform.Scale, mesh_component.GetMesh());
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

		const auto& point_Group = reg->view<TransformComponent, PointLightComponent>();
		for (const auto& entity : point_Group)
		{
			const auto& [transform, comp] = point_Group.get<TransformComponent, PointLightComponent>(entity);

			if (comp.IsActive == 1)
			{
				comp.Position = glm::vec4(transform.WorldPos, 1.0);
				RendererDrawList::SubmitPointLight(dynamic_cast<PointLight*>(&comp));
			}
		}

		const auto& spot_Group = reg->view<TransformComponent, SpotLightComponent>();
		for (const auto& entity : spot_Group)
		{
			const auto& [transform, comp] = spot_Group.get<TransformComponent, SpotLightComponent>(entity);

			if (comp.IsActive == 1)
			{
				comp.Position = glm::vec4(transform.WorldPos, 1.0);
				RendererDrawList::SubmitSpotLight(dynamic_cast<SpotLight*>(&comp));
			}
		}
	}

	void RendererSystem::SubmitMeshes()
	{
		entt::registry* reg = m_World->m_CurrentRegistry;
		Scene* scene = WorldAdmin::GetSingleton()->GetActiveScene();

		const auto& group = reg->view<TransformComponent, MeshComponent>();
		for (const auto& entity : group)
		{
			const auto& [transform, mesh] = group.get<TransformComponent, MeshComponent>(entity);

			if (mesh.bShow == false || mesh.GetMesh() == nullptr)
				continue;

			RendererDrawList::SubmitMesh(transform.WorldPos, transform.Rotation, transform.Scale, mesh.GetMesh(), mesh.GetMeshView());
		}
	}

	void RendererSystem::SubmitSprites()
	{
		entt::registry* reg = m_World->m_CurrentRegistry;

		const auto& group = reg->view<TransformComponent, Texture2DComponent>();
		for (const auto& entity : group)
		{
			const auto& [transform, texture2D] = group.get<TransformComponent, Texture2DComponent>(entity);
			texture2D.LayerIndex = GetLayerIndex(texture2D.LayerIndex);

			if (texture2D.Enabled && texture2D.GetTexture() != nullptr)
			{
				//RendererDrawList2D::SubmitSprite(transform.WorldPos,
				//	transform.Scale, transform.Rotation, texture2D.LayerIndex, texture2D.GetTexture(), true, texture2D.Color);
			}
		}
	}
}