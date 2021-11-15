#pragma once
#include "Window/Events.h"
#include "Renderer/RendererShared.h"

#include <glm/glm.hpp>

namespace SmolEngine
{
	class Camera;

	struct SceneViewProjection
	{
		SceneViewProjection() = default;
		SceneViewProjection(Camera* cam);

		void Update(Camera* cam);

		glm::mat4 Projection = glm::mat4(1.0f);
		glm::mat4 View = glm::mat4(1.0f);
		glm::vec4 CamPos = glm::vec4(1.0f);
		float     NearClip = 0.0f;
		float     FarClip = 0.0f;
		glm::vec2 Pad1;
		glm::mat4 SkyBoxMatrix = glm::mat4(1.0f);

		friend struct RendererStorage;
		friend struct RendererDrawList;
	};

	enum class CameraType : uint32_t
	{
		Perspective,
		Ortho
	};

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

		SceneViewProjection* GetSceneViewProjection() { return &m_SceneViewProjection; }

	private:
		SceneViewProjection m_SceneViewProjection;
	};
}