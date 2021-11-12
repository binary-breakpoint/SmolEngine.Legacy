#include "stdafx.h"
#include "ECS/Systems/ComponentHandler.h"
#include "ECS/Systems/PhysicsSystem.h"
#include "ECS/Systems/AudioSystem.h"

#include "ECS/ComponentsCore.h"
#include "ECS/Components/Singletons/WorldAdminStateSComponent.h"
#include "ECS/Components/Singletons/GraphicsEngineSComponent.h"

namespace SmolEngine
{
	bool ComponentHandler::ValidateMeshComponent(MeshComponent* comp, const std::string& filePath, bool pooling)
	{
		Ref<Mesh> mesh = nullptr;

		if (pooling)
		{
			AssetManager* assetManager = WorldAdmin::GetSingleton()->GetAssetManager();
			assetManager->AddMesh(filePath, comp->RootMesh);
		}

		if (!mesh) { mesh = std::make_shared<Mesh>(); }
		if (!mesh->IsReady())
		{
			Mesh::Create(filePath, mesh.get());
		}

		if (mesh->IsReady())
		{
			auto& gltf_scene = mesh->GetScene();
			uint32_t count = static_cast<uint32_t>(gltf_scene.size());

			comp->Nodes.resize(count);
			comp->RootMesh = mesh;
			comp->FilePath = filePath;
			comp->eType = MeshTypeEX::Custom;

			for (uint32_t i = 0; i < count; ++i)
			{
				comp->Nodes[i].Mesh = gltf_scene[i];
			}

			return true;
		}

		return false;
	}

	bool ComponentHandler::ValidateMeshComponent(MeshComponent* component, MeshTypeEX type)
	{
		if(type == MeshTypeEX::Custom)
			return false;

		auto meshes = GraphicsContext::GetSingleton()->GetDefaultMeshes();
		switch (type)
		{
		case MeshTypeEX::Cube: component->RootMesh = meshes->Cube; break;
		case MeshTypeEX::Sphere: component->RootMesh = meshes->Sphere; break;
		case MeshTypeEX::Capsule: component->RootMesh = meshes->Capsule; break;
		case MeshTypeEX::Torus: component->RootMesh = meshes->Torus; break;
		default:
			return false;
		}

		auto& gltf_scene = component->RootMesh->GetScene();
		uint32_t count = static_cast<uint32_t>(gltf_scene.size());

		component->eType = type;
		component->Nodes.resize(count);
		for (uint32_t i = 0; i < count; ++i)
		{
			component->Nodes[i].Mesh = gltf_scene[i];
		}

		return true;
	}

	bool ComponentHandler::SetMeshMaterial(MeshComponent* comp, uint32_t mesh_index, const std::string& material_path)
	{
		MaterialCreateInfo matInfo = {};
		if (matInfo.Load(material_path))
		{
			GraphicsEngineSComponent* frostium = GraphicsEngineSComponent::Get();
			MaterialLibrary& matetialLib = frostium->Storage.GetMaterialLibrary();

			uint32_t id = matetialLib.Add(&matInfo, material_path);
			frostium->Storage.OnUpdateMaterials();

			comp->Nodes[mesh_index].MaterialPath = material_path;
			comp->Nodes[mesh_index].MaterialID = id;
			return true;
		}

		return false;
	}

	bool ComponentHandler::AddAnimationControllerClip(AnimationControllerComponent* comp, const std::string& path)
	{
		AnimationClipCreateInfo clipCI = {};
		if (clipCI.Load(path))
		{
			comp->AddClip(clipCI, clipCI.Name);
			comp->Paths.emplace_back(path);
			return true;
		}

		return false;
	}

	bool ComponentHandler::ValidateAudioSourceComponent(AudioSourceComponent* comp, const std::string& clip_path)
	{
		AudioClipCreateInfo info{};
		if (info.Load(clip_path))
		{
			comp->Clips.emplace_back(AudioSourceComponent::ClipInstance());
			auto& clip = comp->Clips.back();
			clip.m_CreateInfo = std::move(info);

			AssetManager* assetManager = WorldAdmin::GetSingleton()->GetAssetManager();
			assetManager->AddAudioClip(&clip.m_CreateInfo, clip.m_Clip);
			if (clip.m_CreateInfo.bPlayOnAwake && WorldAdmin::GetSingleton()->IsInPlayMode())
			{
				return AudioSystem::PlayClip(comp, clip.m_Clip, clip.m_Handle);
			}

			return true;
		}

		return false;
	}

	bool ComponentHandler::ValidateTexture2DComponent(Texture2DComponent* comp, const std::string& filePath)
	{
		AssetManager* assetManager = WorldAdmin::GetSingleton()->GetAssetManager();
		auto id = assetManager->AddTexture(filePath, comp->Texture);
		if (id > 0)
		{
			comp->TexturePath = filePath;
			return true;
		}

		return false;
	}

	bool ComponentHandler::ValidateBody2DComponent(Rigidbody2DComponent* comp, Ref<Actor>& actor)
	{
		comp->Actor = actor;
		return true;
	}

	bool ComponentHandler::ValidateRigidBodyComponent(RigidbodyComponent* comp, Ref<Actor>& actor)
	{
		comp->CreateInfo.pActor = actor;
		if (WorldAdmin::GetSingleton()->IsInPlayMode())
		{
			TransformComponent* transform = actor->GetComponent<TransformComponent>();
			comp->Create(&comp->CreateInfo, transform->WorldPos, transform->Rotation);
			PhysicsSystem::AttachBodyToActiveScene(dynamic_cast<RigidActor*>(comp));
		}

		return true;
	}
}