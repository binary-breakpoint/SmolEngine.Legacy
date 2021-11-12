#pragma once
#include "Core/Core.h"

namespace SmolEngine
{
	struct TransformComponentCSharp;
	struct HeadComponentCSharp;
	struct RigidBodyCreateInfoCSharp;

	class CSharpAPi
	{
	public:
		// Actor
		static bool       GetComponent_CSharpAPI(void* ptr, uint32_t entity_id, uint16_t component_type);
		static bool       SetComponent_CSharpAPI(void* ptr, uint32_t entity_id, uint16_t component_type);
		static bool       HasComponent_CSharpAPI(uint32_t entity_id, uint16_t component_type);
		static bool       AddComponent_CSharpAPI(void* ptr, uint32_t entity_id, uint16_t component_type);
		static bool       DestroyComponent_CSharpAPI(uint32_t entity_id, uint16_t component_type);
		static void*      GetEntityName_CSharpAPI(uint32_t entity_id);
		static void*      GetEntityTag_CSharpAPI(uint32_t entity_id);
		static uint32_t   GetActorID_CSharpAPI(uint32_t entity_id);

		// Utils
		static bool       IsKeyInput_CSharpAPI(uint16_t key);
		static bool       IsMouseInput_CSharpAPI(uint16_t button);
		static void       AddMessage_CSharpAPI(void* mono_string, uint32_t level);
		static void       GetMousePos_CSharpAPI(glm::vec2* pos);

		// Prefab
		static uint32_t   PrefabInstantiate_CSharpAPI(size_t id, glm::vec3* pos, glm::vec3* scale, glm::vec3* rot);

		// Aseets
		static size_t     LoadAsset_CSharpAPI(void* str, uint16_t type);

		// Mesh
		static bool       MeshSetMaterial_CSharpAPI(uint32_t mesh_index, uint32_t material_id, uint32_t entity_id);
		static uint32_t   MeshGetChildsCount_CSharpAPI(size_t assetID);
		static bool       SetMesh_CSharpAPI(size_t assetID, uint32_t entity_id);

		// Rigidbody
		static void       RigidBodyCreate_CSharpAPI(RigidBodyCreateInfoCSharp* ptr, uint32_t entity_id);
		static void       RigidBodySetImpact_CSharpAPI(glm::vec3* dir, uint32_t entity_id, uint16_t flags);

		// Scene
		static uint32_t   CreateActor_CSharpAPI(void* name, void* tag);
		static uint32_t   FindActorByName_CSharpAPI(void* name);
		static uint32_t   FindActorByTag_CSharpAPI(void* name);
		static bool       IsActorExistsID_CSharpAPI(uint32_t id);
		static bool       DestroyActor_CSharpAPI(uint32_t id);
	};

}