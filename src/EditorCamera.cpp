#include "stdafx.h"
#include "EditorCamera.h"
#include "Common/Input.h"
#include "Common/Framebuffer.h"
#include "Utils/Utils.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <cmath>

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	EditorCamera::EditorCamera(EditorCameraCreateInfo* createInfo)
	{
		if (createInfo)
		{
			m_FOV = createInfo->FOV;
			m_NearClip = createInfo->NearClip;
			m_FarClip = createInfo->FarClip;
			m_Type = createInfo->Type;
			m_Yaw = createInfo->Yaw;
			m_Pitch = createInfo->Pitch;
			m_FocalPoint = createInfo->WorldPos;
			m_Position = createInfo->WorldPos;
		}

		switch (m_Type)
		{
		case CameraType::Ortho:
		{
			if(createInfo)
				m_Speed = createInfo->Speed != 1.0 ? createInfo->Speed : 1.0f;

			m_Projection = glm::ortho(-m_AspectRatio * m_Distance, m_AspectRatio * m_Distance, -m_Distance, m_Distance, m_NearClip, m_FarClip);
			UpdateViewOrtho();
			break;
		}
		case CameraType::Perspective:
		{
			if (createInfo)
				m_Speed = createInfo->Speed != 1.0 ? createInfo->Speed : 5.0f;

			m_Projection = glm::perspective(glm::radians(m_FOV), m_AspectRatio, m_NearClip, m_FarClip);
			UpdateViewPerspective();
			break;
		}
		}
	}

	void EditorCamera::OnUpdate(DeltaTime delta)
	{
		float speed = m_Speed;
		if (Input::IsKeyPressed(KeyCode::LeftShift))
			speed *= 2;

		switch (m_Type)
		{
		case CameraType::Ortho:
		{
			if (Input::IsKeyPressed(F_KEY_D))
				m_Position.x += speed * delta;

			else if (Input::IsKeyPressed(F_KEY_A))
				m_Position.x -= speed * delta;

			else if (Input::IsKeyPressed(F_KEY_W))
				m_Position.y += speed * delta;

			else if (Input::IsKeyPressed(F_KEY_S))
				m_Position.y -= speed * delta;

			speed = m_Distance;
			UpdateViewOrtho();
			break;
		}
		case CameraType::Perspective:
		{
			if (Input::IsKeyPressed(Key::W))
				m_FocalPoint += (GetForwardDirection() * delta.GetTime()) * speed;

			if (Input::IsKeyPressed(Key::S))
				m_FocalPoint -= (GetForwardDirection() * delta.GetTime()) * speed;

			if (Input::IsKeyPressed(Key::D))
				m_FocalPoint += (GetRightDirection() * delta.GetTime()) * speed;

			if (Input::IsKeyPressed(Key::A))
				m_FocalPoint -= (GetRightDirection() * delta.GetTime()) * speed;

			const glm::vec2& mouse{ Input::GetMouseX(), Input::GetMouseY() };
			glm::vec2 deltaT = (mouse - m_InitialMousePosition) * 0.003f;
			m_InitialMousePosition = mouse;

			if (Input::IsMouseButtonPressed(Mouse::ButtonMiddle))
				MousePan(deltaT);

			if (Input::IsKeyPressed(Key::LeftAlt))
			{
				if (Input::IsMouseButtonPressed(Mouse::ButtonLeft))
				{
					MouseRotate(deltaT);
				}

				if (Input::IsMouseButtonPressed(Mouse::ButtonRight))
					MouseZoom(deltaT.y);
			}

			UpdateViewPerspective();
			break;
		}
		}
		
	}

	void EditorCamera::OnEvent(Event& e)
	{
		if (e.IsType(EventType::MOUSE_SCROLL))
		{
			MouseScrollEvent* scroll = e.Cast<MouseScrollEvent>();
			OnMouseScroll(scroll->GetXoffset(), scroll->GetYoffset());
		}

		if (e.IsType(EventType::WINDOW_RESIZE))
		{
			WindowResizeEvent* resize = e.Cast<WindowResizeEvent>();
			OnResize(resize->GetWidth(), resize->GetHeight());
		}
	}

	void EditorCamera::OnResize(uint32_t width, uint32_t height)
	{
		m_ViewportWidth = static_cast<float>(width);
		m_ViewportHeight = static_cast<float>(height);
		m_AspectRatio = m_ViewportWidth / m_ViewportHeight;

		if (m_ViewportWidth == 0.0f || m_ViewportHeight == 0.0f)
			return;

		SetCameraType(m_Type);
	}

	void EditorCamera::OnMouseScroll(float x, float y)
	{
		switch (m_Type)
		{
		case CameraType::Ortho:
		{
			m_Distance -= y * 0.25f;
			m_Distance = std::max(m_Distance, 0.25f);
			UpdateViewOrtho();
			break;
		}
		case CameraType::Perspective:
		{
			float delta = y * 0.1f;
			MouseZoom(delta);
			UpdateViewPerspective();
			break;
		}
		}

	}

	inline void EditorCamera::SetViewportSize(float width, float height)
	{
		m_ViewportWidth = width;
		m_ViewportHeight = height; 
		UpdateProjection();
	}

	void EditorCamera::SetCameraType(CameraType type)
	{
		m_Type = type;
		switch (m_Type)
		{
		case CameraType::Perspective:
		{
			m_Projection = glm::perspective(glm::radians(m_FOV), m_AspectRatio, m_NearClip, m_FarClip);
			UpdateViewPerspective();
			break;
		}
		case CameraType::Ortho:
		{
			m_Projection = glm::ortho(-m_AspectRatio * m_Distance, m_AspectRatio * m_Distance, -m_Distance, m_Distance, m_NearClip, m_FarClip);
			UpdateViewOrtho();
			break;
		}
		default:
			break;
		}
	}

	void EditorCamera::SetPosition(const glm::vec3& pos)
	{
		switch (m_Type)
		{
		case CameraType::Perspective:
		{
			m_FocalPoint = pos;
			UpdateViewPerspective();
			break;
		}
		case CameraType::Ortho:
		{
			m_Position = pos;
			UpdateViewOrtho();
			break;
		}
		default:
			break;

		}
	}

	void EditorCamera::SetYaw(float value)
	{
		switch (m_Type)
		{
		case CameraType::Perspective:
		{
			m_Yaw = value;
			UpdateViewPerspective();
			break;
		}
		default:
			break;

		}
	}

	void EditorCamera::SetPitch(float value)
	{
		switch (m_Type)
		{
		case CameraType::Perspective:
		{
			m_Pitch = value;
			UpdateViewPerspective();
			break;
		}
		default:
			break;

		}
	}

	glm::vec3 EditorCamera::GetForwardDirection() const
	{
		return glm::rotate(GetOrientation(), glm::vec3(0.0f, 0.0f, -1.0f));
	}

	glm::vec3 EditorCamera::GetRightDirection() const
	{
		return glm::rotate(GetOrientation(), glm::vec3(1.0f, 0.0f, 0.0f));
	}

	glm::vec3 EditorCamera::GetUpDirection() const
	{
		return glm::rotate(GetOrientation(), glm::vec3(0.0f, 1.0f, 0.0f));
	}

	glm::quat EditorCamera::GetOrientation() const
	{
		return glm::quat(glm::vec3(-m_Pitch, -m_Yaw, 0.0f));
	}

	const CameraType EditorCamera::GetType() const
	{
		return m_Type;
	}

	void EditorCamera::UpdateProjection()
	{
		m_AspectRatio = m_ViewportWidth / m_ViewportHeight;
		m_Projection = glm::perspective(glm::radians(m_FOV), m_AspectRatio, m_NearClip, m_FarClip);
	}

	void EditorCamera::UpdateViewPerspective()
	{
		m_Position = CalculatePosition();

		glm::quat orientation = GetOrientation();
		m_ViewMatrix = glm::translate(glm::mat4(1.0f), m_Position) * glm::toMat4(orientation);
		m_ViewMatrix = glm::inverse(m_ViewMatrix);
	}

	void EditorCamera::UpdateViewOrtho()
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_Position) * glm::rotate(glm::mat4(1.0f), glm::radians(m_Pitch), glm::vec3(0, 0, 1));
		m_ViewMatrix = glm::inverse(transform);
		m_Projection = glm::ortho(-m_AspectRatio * m_Distance, m_AspectRatio * m_Distance, -m_Distance, m_Distance, m_NearClip, m_FarClip);
	}

	void EditorCamera::MousePan(const glm::vec2& delta)
	{
		auto [xSpeed, ySpeed] = PanSpeed();
		m_FocalPoint += -GetRightDirection() * delta.x * xSpeed * m_Distance;
		m_FocalPoint += GetUpDirection() * delta.y * ySpeed * m_Distance;
	}

	void EditorCamera::MouseRotate(const glm::vec2& delta)
	{
		float yawSign = GetUpDirection().y;
		m_Yaw += yawSign * delta.x * RotationSpeed();
		m_Pitch += delta.y * RotationSpeed();
	}

	void EditorCamera::MouseZoom(float delta)
	{
		m_Distance -= delta * ZoomSpeed();
		if (m_Distance < 1.0f)
		{
			m_FocalPoint += GetForwardDirection();
			m_Distance = 1.0f;
		}
	}

	float EditorCamera::RotationSpeed() const
	{
		return m_RotationSpeed;
	}

	float EditorCamera::ZoomSpeed() const
	{
		float distance = m_Distance * 0.2f;
		distance = std::max(distance, 0.0f);
		float speed = distance * distance;
		speed = std::min(speed, m_MaxZoomSpeed);
		return speed;
	}

	std::pair<float, float> EditorCamera::PanSpeed() const
	{
		float x = std::min(m_ViewportWidth / 1000.0f, 2.4f); // max = 2.4f
		float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

		float y = std::min(m_ViewportHeight / 1000.0f, 2.4f); // max = 2.4f
		float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

		return { xFactor, yFactor };
	}

	glm::vec3 EditorCamera::CalculatePosition() const
	{
		return m_FocalPoint - GetForwardDirection() * m_Distance;
	}
}