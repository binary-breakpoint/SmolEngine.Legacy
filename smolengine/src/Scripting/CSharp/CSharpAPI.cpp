#include "stdafx.h"

#include "Scripting/CSharp/CSharpAPI.h"
#include "Scripting/CSharp/CSharpDefs.h"
#include "Scripting/CSharp/MonoContext.h"

#include "ECS/Components/Include/Components.h"
#include "ECS/Systems/PhysicsSystem.h"
#include "ECS/Components/Singletons/GraphicsEngineSComponent.h"

#include "Asset/AssetManager.h"
#include "Materials/PBRFactory.h"
#include "Window/Events.h"
#include "Window/Input.h"

#include "Pools/AudioPool.h"
#include "Pools/MeshPool.h"
#include "Pools/TexturePool.h"
#include "Pools/PrefabPool.h"

#include <mono/jit/jit.h>

namespace SmolEngine
{
	enum class ComponentTypeEX : uint16_t
	{
		Transform,
		Camera,
		RigidBody,
		RigidBody2D,
		Mesh,
		PointLight,
		SpotLight,
		Light2D,
		Texture2D,
		RendererState,
		Canvas,
		DirectionalLight,

		MaxEnum
	};

	enum class ImpactFlags : uint16_t
	{
		Force,
		Impulse,
		Torque,
		Gravity,
		Rotation
	};

	template<typename T>
	void CheckHandler(T* component, uint32_t entity_id)
	{
		if (component->Handler == nullptr)
		{
			Scene* scene = WorldAdmin::GetSingleton()->GetActiveScene();
			HeadComponent* head = scene->GetComponent<HeadComponent>((entt::entity)entity_id);
			Ref<Actor> actor = scene->FindActorByID(head->ActorID);
			component->Handler = (uint32_t*)actor.get();
		}
	}

	void SetGetTransform(TransformComponent* native_comp, TransformComponentCSharp* c_comp, bool get)
	{
		if (get)
		{
			c_comp->Scale = native_comp->Scale;
			c_comp->WorldPos = native_comp->WorldPos;
			c_comp->Rotation = native_comp->Rotation;
			return;
		}

		native_comp->Scale = c_comp->Scale;
		native_comp->WorldPos = c_comp->WorldPos;
		native_comp->Rotation = c_comp->Rotation;
	}

	void SetGetMesh_Component(MeshComponent* native_comp, MeshComponentCSharp* c_comp, bool get)
	{
		if (get)
		{
			c_comp->IsVisible = native_comp->bShow;
			c_comp->IsActive = native_comp->GetMesh() != nullptr;
			return;
		}

		native_comp->bShow = c_comp->IsVisible;
	}

	void SetGetCamera_Component(CameraComponent* native_comp, CameraComponentCSharp* c_comp, bool get)
	{
		if (get)
		{
			c_comp->FOV = native_comp->FOV;
			c_comp->zFar = native_comp->zFar;
			c_comp->zNear = native_comp->zNear;
			c_comp->Zoom = native_comp->ZoomLevel;
			c_comp->IsPrimary = native_comp->bPrimaryCamera;
			return;
		}

		native_comp->FOV = c_comp->FOV;
		native_comp->zFar = c_comp->zFar;
		native_comp->zNear = c_comp->zNear;
		native_comp->ZoomLevel = c_comp->Zoom;
		native_comp->bPrimaryCamera = c_comp->IsPrimary;
	}

	void SetGetPointLightComponent(PointLightComponent* native_comp, PointLightComponentCSharp* c_comp, bool get)
	{
		if (get)
		{
			c_comp->Color = native_comp->Color;
			c_comp->Intensity = native_comp->Intensity;
			c_comp->Raduis = native_comp->Raduis;
			c_comp->IsActive = (bool)native_comp->IsActive;
			return;
		}

		native_comp->Color = glm::vec4(c_comp->Color, 1);
		native_comp->Intensity = c_comp->Intensity;
		native_comp->Raduis = c_comp->Raduis;
		native_comp->IsActive = c_comp->IsActive;
	}

	void SetGetDirectionalLightComponent(DirectionalLightComponent* native_comp, DirectionalLightComponentCSharp* c_comp, bool get)
	{
		if (get)
		{
			c_comp->Color = native_comp->Color;
			c_comp->Dir = native_comp->Direction;
			c_comp->Intensity = native_comp->Intensity;
			c_comp->ShadowType = (uint16_t)native_comp->eShadowType;
			c_comp->IsActive = (bool)native_comp->IsActive;
			return;
		}

		native_comp->Color = glm::vec4(c_comp->Color, 1);
		native_comp->Direction = glm::vec4(c_comp->Dir, 1);
		native_comp->Intensity = c_comp->Intensity;
		native_comp->eShadowType = (ShadowType)c_comp->ShadowType;
		native_comp->IsCastShadows = native_comp->eShadowType == ShadowType::Hard || native_comp->eShadowType == ShadowType::Soft;
		native_comp->IsActive = c_comp->IsActive;

		RendererDrawList::SubmitDirLight(dynamic_cast<DirectionalLight*>(native_comp));
	}

	bool ModifyComponent(void* ptr, uint32_t entity_id, uint16_t component_type, bool get_flag, bool add_flag = false)
	{
		ComponentTypeEX type = (ComponentTypeEX)component_type;
		Scene* scene = WorldAdmin::GetSingleton()->GetActiveScene();
		entt::entity id = (entt::entity)entity_id;
		switch (type)
		{
		case ComponentTypeEX::Transform:
		{
			auto* native_comp = add_flag == false ? scene->GetComponent<TransformComponent>(id): scene->AddComponent<TransformComponent>(id);
			auto* c_comp = (TransformComponentCSharp*)ptr;
			if (native_comp)
			{
				CheckHandler<TransformComponentCSharp>(c_comp, entity_id);
				SetGetTransform(native_comp, c_comp, get_flag);
			}

			return native_comp != nullptr;
		}
		case ComponentTypeEX::Mesh:
		{
			auto* native_comp = add_flag == false ?  scene->GetComponent<MeshComponent>(id): scene->AddComponent<MeshComponent>(id);
			auto* c_comp = (MeshComponentCSharp*)ptr;
			if (native_comp)
			{
				CheckHandler<MeshComponentCSharp>(c_comp, entity_id);
				SetGetMesh_Component(native_comp, c_comp, get_flag);
			}

			return native_comp != nullptr;
		}
		case ComponentTypeEX::RigidBody:
		{
			auto* native_comp = add_flag == false ? scene->GetComponent<RigidbodyComponent>(id) : scene->AddComponent<RigidbodyComponent>(id);
			auto* c_comp = (RigidBodyComponentCSharp*)ptr;
			if (native_comp)
			{
				CheckHandler<RigidBodyComponentCSharp>(c_comp, entity_id);
			}

			return native_comp != nullptr;
		}
		case ComponentTypeEX::Camera:
		{
			auto* native_comp = add_flag == false ? scene->GetComponent<CameraComponent>(id) : scene->AddComponent<CameraComponent>(id);
			auto* c_comp = (CameraComponentCSharp*)ptr;
			if (native_comp)
			{
				CheckHandler<CameraComponentCSharp>(c_comp, entity_id);
				SetGetCamera_Component(native_comp, c_comp, get_flag);
			}

			return native_comp != nullptr;
		}
		case ComponentTypeEX::PointLight:
		{
			auto* native_comp = add_flag == false ? scene->GetComponent<PointLightComponent>(id) : scene->AddComponent<PointLightComponent>(id);
			auto* c_comp = (PointLightComponentCSharp*)ptr;
			if (native_comp)
			{
				CheckHandler<PointLightComponentCSharp>(c_comp, entity_id);
				SetGetPointLightComponent(native_comp, c_comp, get_flag);
			}

			return native_comp != nullptr;
		}
		case ComponentTypeEX::DirectionalLight:
		{
			auto* native_comp = add_flag == false ? scene->GetComponent<DirectionalLightComponent>(id) : scene->AddComponent<DirectionalLightComponent>(id);
			auto* c_comp = (DirectionalLightComponentCSharp*)ptr;
			if (native_comp)
			{
				CheckHandler<DirectionalLightComponentCSharp>(c_comp, entity_id);
				SetGetDirectionalLightComponent(native_comp, c_comp, get_flag);
			}

			return native_comp != nullptr;
		}
		}

		return false;
	}

	bool CSharpAPi::GetComponent_CSharpAPI(void* ptr, uint32_t entity_id, uint16_t component_type)
	{
		const bool get_flag = true;
		return ModifyComponent(ptr, entity_id, component_type, get_flag);
	}

	bool CSharpAPi::SetComponent_CSharpAPI(void* ptr, uint32_t entity_id, uint16_t component_type)
	{
		const bool get_flag = false;
		return ModifyComponent(ptr, entity_id, component_type, get_flag);
	}

	bool CSharpAPi::AddComponent_CSharpAPI(void* ptr, uint32_t entity_id, uint16_t component_type)
	{
		const bool get_flag = true;
		const bool add_flag = true;
		return ModifyComponent(ptr, entity_id, component_type, get_flag, add_flag);
	}

	bool CSharpAPi::DestroyComponent_CSharpAPI(uint32_t entity_id, uint16_t component_type)
	{
		ComponentTypeEX type = (ComponentTypeEX)component_type;
		Scene* scene = WorldAdmin::GetSingleton()->GetActiveScene();
		entt::entity id = (entt::entity)entity_id;

		switch (type)
		{
		case ComponentTypeEX::Transform: return scene->DestroyComponent<TransformComponent>(id);
		case ComponentTypeEX::Mesh: return scene->DestroyComponent<MeshComponent>(id);
		case ComponentTypeEX::RigidBody: return scene->DestroyComponent<RigidbodyComponent>(id);
		case ComponentTypeEX::RigidBody2D: return scene->DestroyComponent<Rigidbody2DComponent>(id);
		case ComponentTypeEX::Camera: return scene->DestroyComponent<CameraComponent>(id);
		case ComponentTypeEX::PointLight: return scene->DestroyComponent<PointLightComponent>(id);
		case ComponentTypeEX::SpotLight: return scene->DestroyComponent<SpotLightComponent>(id);
		case ComponentTypeEX::Texture2D: return scene->DestroyComponent<Texture2DComponent>(id);
		case ComponentTypeEX::DirectionalLight: return scene->DestroyComponent<DirectionalLightComponent>(id);
		}

		return false;
	}

	bool CSharpAPi::HasComponent_CSharpAPI(uint32_t entity_id, uint16_t component_type)
	{
		ComponentTypeEX type = (ComponentTypeEX)component_type;
		Scene* scene = WorldAdmin::GetSingleton()->GetActiveScene();
		entt::entity id = (entt::entity)entity_id;

		switch (type)
		{
		case ComponentTypeEX::Transform: return scene->HasComponent<TransformComponent>(id);
		case ComponentTypeEX::Mesh: return scene->HasComponent<MeshComponent>(id);
		case ComponentTypeEX::RigidBody: return scene->HasComponent<RigidbodyComponent>(id);
		case ComponentTypeEX::RigidBody2D: return scene->HasComponent<Rigidbody2DComponent>(id);
		case ComponentTypeEX::Camera: return scene->HasComponent<CameraComponent>(id);
		case ComponentTypeEX::PointLight: return scene->HasComponent<PointLightComponent>(id);
		case ComponentTypeEX::SpotLight: return scene->HasComponent<SpotLightComponent>(id);
		case ComponentTypeEX::Texture2D: return scene->HasComponent<Texture2DComponent>(id);
		case ComponentTypeEX::DirectionalLight: return scene->HasComponent<DirectionalLightComponent>(id);
		}

		return false;
	}

	void* CSharpAPi::GetEntityName_CSharpAPI(uint32_t entity_id)
	{
		Scene* scene = WorldAdmin::GetSingleton()->GetActiveScene();
		MonoDomain* domain = MonoContext::GetSingleton()->GetDomain();

		HeadComponent* head = scene->GetComponent<HeadComponent>((entt::entity)entity_id);
		MonoString* str = mono_string_new(domain, head->Name.c_str());
		return str;
	}

	void* CSharpAPi::GetEntityTag_CSharpAPI(uint32_t entity_id)
	{
		Scene* scene = WorldAdmin::GetSingleton()->GetActiveScene();
		MonoDomain* domain = MonoContext::GetSingleton()->GetDomain();

		HeadComponent* head = scene->GetComponent<HeadComponent>((entt::entity)entity_id);
		MonoString* str = mono_string_new(domain, head->Tag.c_str());
		return str;
	}

	uint32_t CSharpAPi::GetActorID_CSharpAPI(uint32_t entity_id)
	{
		Scene* scene = WorldAdmin::GetSingleton()->GetActiveScene();
		HeadComponent* head = scene->GetComponent<HeadComponent>((entt::entity)entity_id);
		if (head)
		{
			return head->ActorID;
		}

		return 0;
	}

	bool CSharpAPi::IsKeyInput_CSharpAPI(uint16_t key)
	{
		return Input::IsKeyPressed((KeyCode)key);
	}

	bool CSharpAPi::IsMouseInput_CSharpAPI(uint16_t button)
	{
		return Input::IsMouseButtonPressed((MouseCode)button);
	}

	void CSharpAPi::AddMessage_CSharpAPI(void* mono_string, uint32_t level)
	{
		MonoString* mono_str = (MonoString*)mono_string;
		char* cpp_str = mono_string_to_utf8(mono_str);

		switch (level)
		{
		case 0: DebugLog::LogInfo(cpp_str); break;
		case 1: DebugLog::LogWarn(cpp_str); break;
		case 2: DebugLog::LogError(cpp_str); break;
		}

		mono_free(cpp_str);
	}

	void CSharpAPi::GetMousePos_CSharpAPI(glm::vec2* pos)
	{
		pos->x = Input::GetMouseX();
		pos->y = Input::GetMouseY();
	}

	uint32_t CSharpAPi::PrefabInstantiate_CSharpAPI(size_t id, glm::vec3* pos, glm::vec3* scale, glm::vec3* rot)
	{
		Ref<Prefab> prefab = AssetManager::GetAssetByID<Prefab>(id);
		if (prefab)
		{
			Ref<Actor> actor = WorldAdmin::GetSingleton()->GetActiveScene()->InstantiatePrefab(prefab, *pos, *rot, *scale);
			if (actor)
			{
				return *(uint32_t*)actor.get();
			}
		}

		return 0;
	}

	size_t CSharpAPi::LoadAsset_CSharpAPI(void* str, uint16_t type)
	{
		size_t id = 0;
		AssetType asset_type = (AssetType)type;
		char* filepath = mono_string_to_utf8((MonoString*)str);

		if (std::filesystem::exists(filepath))
		{
			switch (asset_type)
			{
			case AssetType::Prefab:
			{
				Ref<Prefab> prefab = PrefabPool::ConstructFromPath(filepath);
				id = prefab->GetUUID();
				break;
			}
			case AssetType::Mesh:
			{
				auto& [mesh, view] = MeshPool::ConstructFromFile(filepath);
				id = mesh->GetUUID();
				break;
			}
			case AssetType::Texture:
			{
				TextureCreateInfo texCI{};
				if (texCI.Load(filepath))
				{
					Ref<Texture> texture = TexturePool::ConstructFromFile(&texCI);
					id = texture->GetUUID();
				}
				break;
			}
			case AssetType::Material: // FIX
			{
				PBRCreateInfo info;
				if (info.Load(filepath))
				{
					PBRFactory::AddMaterial(&info, filepath);
					id = 0;
				}

				break;
			}
			}
		}

		mono_free(filepath);
		return id;
	}

	bool CSharpAPi::MeshSetMaterial_CSharpAPI(uint32_t mesh_index, uint32_t material_id, uint32_t entity_id) // rework
	{
		Scene* scene = WorldAdmin::GetSingleton()->GetActiveScene();
		MeshComponent* comp = scene->GetComponent<MeshComponent>((entt::entity)entity_id);
		return false;
	}

	uint32_t CSharpAPi::MeshGetChildsCount_CSharpAPI(size_t assetID)
	{
		Ref<Mesh> mesh = AssetManager::GetAssetByID<Mesh>(assetID);
		if (mesh)
			return mesh->GetChildCount();
		
		return 0;
	}

	bool CSharpAPi::SetMesh_CSharpAPI(size_t assetID, uint32_t entity_id) // remove
	{
		return false;
	}

	void CSharpAPi::RigidBodyCreate_CSharpAPI(RigidBodyCreateInfoCSharp* ptr, uint32_t entity_id)
	{
		Scene* scene = WorldAdmin::GetSingleton()->GetActiveScene();
		entt::entity id = (entt::entity)entity_id;

		HeadComponent* head = scene->GetComponent<HeadComponent>(id);
		Ref<Actor> actor = scene->FindActorByID(head->ActorID);
		RigidbodyComponent* component = scene->GetComponent<RigidbodyComponent>(id);
		if (!component) { component = scene->AddComponent<RigidbodyComponent>(id); }

		BodyCreateInfo info = {};
		info.eShape = (RigidBodyShape)ptr->eShape;
		info.eType = (RigidBodyType)ptr->eType;
		info.Size = ptr->Size;
		info.Mass = ptr->Mass;
		info.Density = ptr->Density;
		info.Friction = ptr->Friction;
		info.Restitution = ptr->Restitution;
		info.LinearDamping = ptr->LinearDamping;
		info.AngularDamping = ptr->AngularDamping;
		info.RollingFriction = ptr->RollingFriction;
		info.SpinningFriction = ptr->SpinningFriction;

		component->CreateInfo = info;
		component->Validate(actor);
	}

	void CSharpAPi::RigidBodySetImpact_CSharpAPI(glm::vec3* value, uint32_t entity_id, uint16_t flags)
	{
		ImpactFlags flag_ = (ImpactFlags)flags;
		Scene* scene = WorldAdmin::GetSingleton()->GetActiveScene();
		RigidbodyComponent* rb = scene->GetComponent<RigidbodyComponent>((entt::entity)entity_id);
		if (rb)
		{
			switch (flag_)
			{
			case ImpactFlags::Force: rb->AddForce(*value); break;
			case ImpactFlags::Impulse: rb->AddImpulse(*value); break;
			case ImpactFlags::Torque: rb->AddTorque(*value); break;
			case ImpactFlags::Rotation: rb->AddRotation(*value); break;
			case ImpactFlags::Gravity: rb->SetGravity(*value); break;
			}
		}
	}

	uint32_t CSharpAPi::CreateActor_CSharpAPI(void* name, void* tag)
	{
		MonoString* mono_name = (MonoString*)name;
		MonoString* mono_tag = (MonoString*)tag;

		char* cpp_name = mono_string_to_utf8(mono_name);
		char* cpp_tag = mono_string_to_utf8(mono_tag);

		Ref<Actor> actor = WorldAdmin::GetSingleton()->GetActiveScene()->CreateActor(cpp_name, cpp_tag);
		mono_free(cpp_name);
		mono_free(cpp_tag);

		if (actor)
		{
			return *(uint32_t*)actor.get();
		}

		return 0;
	}

	uint32_t CSharpAPi::FindActorByName_CSharpAPI(void* name)
	{
		MonoString* mono_str = (MonoString*)name;
		char* cpp_str = mono_string_to_utf8(mono_str);
		Ref<Actor> actor = WorldAdmin::GetSingleton()->GetActiveScene()->FindActorByName(cpp_str);

		mono_free(cpp_str);
		if (actor)
		{
			return *(uint32_t*)actor.get();
		}

		return 0;
	}

	uint32_t CSharpAPi::FindActorByTag_CSharpAPI(void* name)
	{
		MonoString* mono_str = (MonoString*)name;
		char* cpp_str = mono_string_to_utf8(mono_str);
		Ref<Actor> actor = WorldAdmin::GetSingleton()->GetActiveScene()->FindActorByTag(cpp_str);

		mono_free(cpp_str);
		if (actor)
		{
			return *(uint32_t*)actor.get();
		}

		return 0;
	}

	bool CSharpAPi::IsActorExistsID_CSharpAPI(uint32_t id)
	{
		Ref<Actor> actor = WorldAdmin::GetSingleton()->GetActiveScene()->FindActorByID(id);
		return actor != nullptr;
	}

	bool CSharpAPi::DestroyActor_CSharpAPI(uint32_t id)
	{
		Scene* scene = WorldAdmin::GetSingleton()->GetActiveScene();
		Ref<Actor> actor = scene->FindActorByID(id);
		return scene->DeleteActor(actor);
	}
}