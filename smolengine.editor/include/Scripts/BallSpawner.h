#pragma once

#include "SmolEngineCore.h"

namespace SmolEngine
{
	class BallSpawner : public BehaviourPrimitive
	{
	public:
		float* testValue = nullptr;

		BallSpawner()
		{
			CreateValue<float>(222.0f, "TestValue");
		}

		void OnProcess(float deltaTime)
		{

			if (Input::IsKeyPressed(KeyCode::J))
			{
				glm::vec3 pos = GetComponent<TransformComponent>()->WorldPos;
				auto ball = WorldAdmin::GetSingleton()->GetActiveScene()->CreateActor();

				auto transform = ball->GetComponent<TransformComponent>();
				transform->WorldPos = pos;

				auto mesh = ball->AddComponent<MeshComponent>();
				ComponentHandler::ValidateMeshComponent(mesh, MeshTypeEX::Sphere);

				auto rb = ball->AddComponent<RigidbodyComponent>();
				rb->CreateInfo.eShape = RigidBodyShape::Sphere;
				rb->CreateInfo.eType = RigidBodyType::Dynamic;
				ComponentHandler::ValidateRigidBodyComponent(rb, ball);

				m_ID++;
			}
		}

		void OnBegin() 
		{
			testValue = GetValue<float>("TestValue");
			NATIVE_WARN("Test value is {}", *testValue);
		}
		void OnDestroy() {}
		void OnCollisionContact() {}
		void OnCollisionExit() {}

	private:

		int m_ID = 0;
	};
}