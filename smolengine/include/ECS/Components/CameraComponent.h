#pragma once

#include "ECS/Components/BaseComponent.h"
#include "Camera/Camera.h"

namespace cereal
{
	class access;
}

namespace SmolEngine
{
	struct TransformComponent;

	enum class CameraComponentType : int
	{
		Perspective,
		Ortho
	};

	struct CameraComponent: public BaseComponent
	{
		CameraComponent();
		CameraComponent(uint32_t id);

		void        CalculateView(TransformComponent* transform);
		void        UpdateViewPerspective(TransformComponent* transform);
		void        UpdateViewOrtho(TransformComponent* transform);
		static void OnResize(uint32_t width, uint32_t height);

		glm::mat4   ProjectionMatrix = glm::mat4(1.0f);
		glm::mat4   ViewMatrix = glm::mat4(1.0f);
		float       AspectRatio = 1.0;
		float       ZoomLevel = 6.0f;
		float       zNear = 0.1f;
		float       zFar = 1000.0f;
		int         ImGuiType = 0;
		float       FOV = 75.0f;
		bool        bPrimaryCamera = false;
		CameraType  eType = CameraType::Perspective;

	private:
		friend class cereal::access;
		friend class EditorLayer;
		friend class WorldAdmin;

		template<typename Archive>
		void serialize(Archive& archive)
		{
			archive(AspectRatio, ZoomLevel, bPrimaryCamera, eType, zNear, zFar, FOV, ImGuiType, ComponentID);
		}
	};
}