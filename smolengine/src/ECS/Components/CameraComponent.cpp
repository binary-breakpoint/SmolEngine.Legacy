#include "stdafx.h"
#include "ECS/Components/CameraComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/Singletons/WorldAdminStateSComponent.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace SmolEngine
{
	CameraComponent::CameraComponent()
	{
		auto& spec = GraphicsContext::GetSingleton()->GetMainFramebuffer()->GetSpecification();
		AspectRatio = (float)spec.Width / (float)spec.Height;
	}

	CameraComponent::CameraComponent(uint32_t id)
		:BaseComponent(id)
	{
		auto& spec = GraphicsContext::GetSingleton()->GetMainFramebuffer()->GetSpecification();
		AspectRatio = (float)spec.Width / (float)spec.Height;
	}

	void CameraComponent::CalculateView(TransformComponent* transform)
	{
		auto& spec = GraphicsContext::GetSingleton()->GetMainFramebuffer()->GetSpecification();
		AspectRatio = static_cast<float>(spec.Width) / static_cast<float>(spec.Height);

		switch (eType)
		{
		case CameraType::Ortho:        UpdateViewOrtho(transform); break;
		case CameraType::Perspective:  UpdateViewPerspective(transform); break;
		}
	}

	void CameraComponent::UpdateViewPerspective(TransformComponent* transform)
	{
		glm::quat orientation = glm::quat(transform->Rotation);
		ViewMatrix = glm::translate(glm::mat4(1.0f), transform->WorldPos) * glm::toMat4(orientation);
		ViewMatrix = glm::inverse(ViewMatrix);

		ProjectionMatrix = glm::perspective(glm::radians(FOV), AspectRatio, zNear, zFar);
	}

	void CameraComponent::UpdateViewOrtho(TransformComponent* transform)
	{
		glm::mat4 mat = glm::translate(glm::mat4(1.0f), transform->WorldPos) * glm::rotate(glm::mat4(1.0f), glm::radians(transform->Rotation.x), glm::vec3(0, 0, 1));

		ViewMatrix = glm::inverse(mat);
		ProjectionMatrix = glm::ortho(-AspectRatio * ZoomLevel, AspectRatio * ZoomLevel, -ZoomLevel, ZoomLevel, zNear, zFar);
	}

	void CameraComponent::OnResize(uint32_t width, uint32_t height)
	{
		entt::registry* reg = WorldAdminStateSComponent::GetSingleton()->m_CurrentRegistry;

		const auto& group = reg->view<CameraComponent, TransformComponent>();
		for (const auto& entity : group)
		{
			auto& [camera, transform] = group.get<CameraComponent, TransformComponent>(entity);
			camera.AspectRatio = static_cast<float>(width) / static_cast<float>(height);
			camera.CalculateView(&transform);
		}
	}
}