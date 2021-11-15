#include "stdafx.h"
#include "ECS/Components/RigidbodyComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Systems/PhysicsSystem.h"

#include <btBulletDynamicsCommon.h>

namespace SmolEngine
{
	RigidbodyComponent::RigidbodyComponent(uint32_t id)
		:BaseComponent(id), RigidBody()
	{
	
	}

	void RigidbodyComponent::Validate(const Ref<Actor>& actor)
	{
		CreateInfo.pActor = actor;

		if (WorldAdmin::GetSingleton()->IsInPlayMode())
		{
			TransformComponent* transform = actor->GetComponent<TransformComponent>();
			Create(&CreateInfo, transform->WorldPos, transform->Rotation);

			PhysicsSystem::AttachBodyToActiveScene(dynamic_cast<RigidActor*>(this));
		}
	}

	void RigidbodyComponent::SetLinearVelocity(const glm::vec3& dir)
	{
		btRigidBody* rb = m_Body;

		rb->activate(true);
		rb->setLinearVelocity(btVector3(dir.x, dir.y, dir.z));
	}

	void RigidbodyComponent::SetAngularFactor(const glm::vec3& axis)
	{
		btRigidBody* rb = m_Body;

		rb->activate(true);
		rb->setAngularFactor(btVector3(axis.x, axis.y, axis.z));
	}

	void RigidbodyComponent::SetGravity(const glm::vec3& gravity)
	{
		btRigidBody* rb = m_Body;

		rb->activate(true);
		rb->setGravity(btVector3(gravity.x, gravity.y, gravity.z));
	}

	void RigidbodyComponent::AddRotation(const glm::vec3& eulerAngles)
	{
		btRigidBody* rb = m_Body;
		rb->activate(true);

		auto& transform = rb->getWorldTransform();
		btQuaternion rotation{};
		rotation.setEuler(eulerAngles.y, eulerAngles.x, eulerAngles.z);
		rotation *= transform.getRotation();

		transform.setRotation(rotation);
	}

	void RigidbodyComponent::AddForce(const glm::vec3& dir)
	{
		btRigidBody* rb = m_Body;

		rb->activate(true);
		rb->applyCentralForce(btVector3(dir.x, dir.y, dir.z));
	}

	void RigidbodyComponent::AddImpulse(const glm::vec3& dir)
	{
		btRigidBody* rb = m_Body;

		rb->activate(true);
		rb->applyCentralImpulse(btVector3(dir.x, dir.y, dir.z));
	}

	void RigidbodyComponent::AddTorque(const glm::vec3& torque)
	{
		btRigidBody* rb = m_Body;

		rb->activate(true);
		rb->applyTorque(btVector3(torque.x, torque.y, torque.z));
	}
}