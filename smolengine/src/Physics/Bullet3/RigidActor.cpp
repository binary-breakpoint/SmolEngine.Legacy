#include "stdafx.h"
#include "Physics/Bullet3/RigidActor.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/Singletons/Bullet3WorldSComponent.h"

#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>
#include <BulletCollision/CollisionShapes/btShapeHull.h>

#include "Tools/Utils.h"
#include "Import/glTFImporter.h"


#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/euler_angles.hpp>

namespace SmolEngine
{
	void RigidActor::GLMToBulletTransform(const glm::vec3& pos, const glm::vec3& rot, btTransform* transform)
	{
		glm::mat4 model;
		Utils::ComposeTransform(pos, rot, { 1,1, 1 }, model);

		transform->setIdentity();
		transform->setFromOpenGLMatrix(glm::value_ptr(model));
	}

	void RigidActor::BulletToGLMTransform(const btTransform* transform, glm::vec3& pos, glm::vec3& rot)
	{
		const auto& origin = transform->getOrigin();
		float x, y, z;
		{
			const auto& rotation = transform->getRotation();
			const glm::mat4 m = glm::toMat4(glm::quat(rotation.getW(), rotation.getX(), rotation.getY(), rotation.getZ()));
			glm::extractEulerAngleXYZ(m, x, y, z);
		}

		pos.x = origin.x();
		pos.y = origin.y();
		pos.z = origin.z();
		rot = glm::vec3(x, y, z);
	}

	bool RigidActor::IsActive() const
	{
		return m_Active;
	}

	void RigidActor::InitBase(BodyCreateInfo* info)
	{
		switch (info->eShape)
		{
		case RigidBodyShape::Box:     CreateBox(info); break;
		case RigidBodyShape::Sphere:  CreateSphere(info); break;
		case RigidBodyShape::Capsule: CreateCapsule(info); break;
		case RigidBodyShape::Convex:  CreateConvex(info); break;
		}

		if (info->eType == RigidBodyType::Static)
			info->Mass = 0.0f;

		bool isDynamic = info->Mass != 0.0f;
		btVector3 inertia{};
		if(isDynamic)
			m_Shape->calculateLocalInertia(info->Mass, inertia);

		info->LocalInertia.x = inertia.x();
		info->LocalInertia.y = inertia.y();
		info->LocalInertia.z = inertia.z();
	}

	void RigidActor::CreateCapsule(BodyCreateInfo* info)
	{
		m_Shape = new btCapsuleShape(info->Size.x, info->Size.y);
	}

	void RigidActor::CreateSphere(BodyCreateInfo* info)
	{
		m_Shape = new btSphereShape(info->Size.x);
	}

	void RigidActor::CreateBox(BodyCreateInfo* info)
	{
		m_Shape = new btBoxShape(btVector3(info->Size.x, info->Size.y, info->Size.z));
	}

	void RigidActor::CreateConvex(BodyCreateInfo* info)
	{
		if (info->FilePath.empty() == false)
		{
			ImportedDataGlTF* data = new ImportedDataGlTF();

			glm::mat4 actor_model{};
			{
				auto transform = info->pActor->GetComponent<TransformComponent>();
				Utils::ComposeTransform({ 0, 0, 0 }, transform->Rotation, transform->Scale, actor_model);
			}

			if (glTFImporter::Import(info->FilePath, data))
			{
				auto& model = data->Primitives[0];
				auto trimesh = new btTriangleMesh();
				for (uint32_t i = 0; i < static_cast<uint32_t>(model.IndexBuffer.size()); i+=3)
				{
					uint32_t index0 = model.IndexBuffer[i];
					uint32_t index1 = model.IndexBuffer[i + 1];
					uint32_t index2 = model.IndexBuffer[i + 2];

					glm::vec3 v0 = glm::vec3(model.VertexBuffer[index0].Pos.x, model.VertexBuffer[index0].Pos.y, model.VertexBuffer[index0].Pos.z);
					glm::vec3 v1 = glm::vec3(model.VertexBuffer[index1].Pos.x, model.VertexBuffer[index1].Pos.y, model.VertexBuffer[index1].Pos.z);
					glm::vec3 v2 = glm::vec3(model.VertexBuffer[index2].Pos.x, model.VertexBuffer[index2].Pos.y, model.VertexBuffer[index2].Pos.z);

					v0 = actor_model * glm::vec4(v0, 1);
					v1 = actor_model * glm::vec4(v1, 1);
					v2 = actor_model * glm::vec4(v2, 1);

					btVector3 vertex0(v0.x, v0.y, v0.z);
					btVector3 vertex1(v1.x, v1.y, v1.z);
					btVector3 vertex2(v2.x, v2.y, v2.z);

					trimesh->addTriangle(vertex0, vertex1, vertex2);
				}

				auto trimeshShape = new btBvhTriangleMeshShape(trimesh, true);
				m_Shape = trimeshShape;
			}

			delete data;
		}
	}

	void RigidActor::SetActive(bool value)
	{
		m_Active = value;
	}
}