#pragma once
#include "Core/Core.h"

#include <glm/glm.hpp>

namespace SmolEngine
{
	struct Bullet3WorldSComponent;
	struct WorldAdminStateSComponent;
	struct RigidbodyComponent;

	class RigidActor;
	class Actor;

	class PhysicsSystem
	{
		static void OnBeginWorld();
		static void OnEndWorld();
		static void OnUpdate(float delta);
		static void OnDestroy(RigidbodyComponent* component);
		static void OnDestroy(Ref<Actor>& actor);

		static void UpdateTransforms();
		static void AttachBodyToActiveScene(RigidActor* body);

	private:

		inline static Bullet3WorldSComponent*       m_State = nullptr;
		inline static WorldAdminStateSComponent*    m_World = nullptr;

		friend struct RigidbodyComponent;
		friend class WorldAdmin;
		friend class Scene;
	};
}