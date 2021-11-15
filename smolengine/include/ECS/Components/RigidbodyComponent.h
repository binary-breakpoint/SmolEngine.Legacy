#pragma once
#include "ECS/Components/BaseComponent.h"
#include "Physics/Bullet3/RigidBody.h"

namespace cereal
{
	class access;
}

namespace SmolEngine
{
	class Actor;
	struct Bullet3WorldSComponent;

	struct RigidbodyComponent : public BaseComponent, RigidBody
	{
		RigidbodyComponent() = default;
		RigidbodyComponent(uint32_t id);

		void Validate(const Ref<Actor>& actor);

		void SetLinearVelocity(const glm::vec3& dir);
		void SetAngularFactor(const glm::vec3& axis);
		void SetGravity(const glm::vec3& gravity);

		void AddRotation(const glm::vec3& eulerAngles);
		void AddForce(const glm::vec3& dir);
		void AddImpulse(const glm::vec3& dir);
		void AddTorque(const glm::vec3& torque);

		BodyCreateInfo  CreateInfo{};

	private:
		friend class cereal::access;

		template<typename Archive>
		void serialize(Archive& archive)
		{
			archive(CreateInfo, ComponentID);
		}
	};
}