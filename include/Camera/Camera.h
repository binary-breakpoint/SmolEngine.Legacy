#pragma once
#include "Window/Events.h"

#include <glm/glm.hpp>

namespace SmolEngine
{
	class Camera
	{
	public:

		Camera() = default;
		virtual ~Camera() = default;

		virtual void OnUpdate(float deltaTime) = 0;
		virtual void OnResize(uint32_t width, uint32_t height) = 0;
		virtual void OnEvent(Event& e) {};

		virtual const glm::mat4 GetViewProjection() const = 0;
		virtual const glm::mat4& GetProjection() const = 0;
		virtual const glm::mat4& GetViewMatrix() const = 0;
		virtual const glm::vec3& GetPosition() const = 0;

		virtual float GetNearClip() const = 0;
		virtual float GetFarClip() const = 0;
	};
}