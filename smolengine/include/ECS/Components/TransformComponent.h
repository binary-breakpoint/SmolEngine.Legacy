#pragma once
#include "ECS/Components/BaseComponent.h"

#include <glm/glm.hpp>
#include <entt/signal/fwd.hpp>
#include <entt/signal/sigh.hpp>

namespace cereal
{
	class access;
}

namespace SmolEngine
{
	class Actor;

	struct TransformComponent: public BaseComponent
	{
		TransformComponent() = default;
		TransformComponent(uint32_t id)
			:BaseComponent(id) {}

		glm::vec3  WorldPos = glm::vec3(0.0f);
		glm::vec3  Rotation = glm::vec3(0.0f);;
		glm::vec3  Scale = glm::vec3(1.0f);
		glm::vec3  DeltaPos = glm::vec3(0.0f);

		// Signals
		entt::sigh<void(Actor* parent)> OnUpdate;
		entt::sink<void(Actor* parent)> TSender{ OnUpdate };

	private:
		friend class cereal::access;
		friend class Body2D;
		friend class Scene;
		friend class Actor;

		template<typename Archive>
		void serialize(Archive& archive)
		{
			archive(Rotation.x, Rotation.y, Rotation.z, Scale.x, Scale.y, Scale.z, WorldPos.x, WorldPos.y, WorldPos.z, DeltaPos.x, DeltaPos.y, DeltaPos.z, ComponentID);
		}
	};
}