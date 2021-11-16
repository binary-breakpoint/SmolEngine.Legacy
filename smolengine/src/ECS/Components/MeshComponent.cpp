#include "stdafx.h"
#include "ECS/Components/MeshComponent.h"

namespace SmolEngine
{
	void MeshComponent::LoadMesh(MeshTypeEX type)
	{
		switch (type)
		{
		case MeshTypeEX::Cube:
		{
			auto& [mesh, view] = MeshPool::GetCube();
			Mesh = mesh;
			View = view;
			break;
		}
		case MeshTypeEX::Sphere:
		{
			auto& [mesh, view] = MeshPool::GetSphere();
			Mesh = mesh;
			View = view;
			break;
		}
		case MeshTypeEX::Capsule:
		{
			auto& [mesh, view] = MeshPool::GetCapsule();
			Mesh = mesh;
			View = view;
			break;
		}
		case MeshTypeEX::Torus:
		{
			auto& [mesh, view] = MeshPool::GetTorus();
			Mesh = mesh;
			View = view;
			break;
		}
		}

		eType = type;
		FilePath = "";
		AnimContoller = nullptr;
	}

	void MeshComponent::LoadMesh(const std::string& path)
	{
		auto& [mesh, view] = MeshPool::ConstructFromFile(path);

		FilePath = path;
		eType = MeshTypeEX::Custom;
		Mesh = mesh;
		View = view;
		AnimContoller = nullptr;
	}

	void MeshComponent::LoadPBRHandle(const std::string& path, uint32_t nodeIndex)
	{
		PBRCreateInfo createInfo{};
		if (createInfo.Load(path))
		{
			const auto& handle = PBRFactory::AddMaterial(&createInfo, path);

			View->SetPBRHandle(handle, nodeIndex);
		}
	}

	void MeshComponent::LoadAnimation(const std::string& path)
	{
		AnimationClipCreateInfo createInfo{};
		if (createInfo.Load(path))
		{
			AnimContoller->AddClip(createInfo, createInfo.Name);
			AnimPaths.emplace_back(path);
		}
	}

	void MeshComponent::SetMaterial(const Ref<Material3D>& material, uint32_t nodeIndex)
	{
		View->SetMaterial(material, nodeIndex);
	}

	void MeshComponent::SetAnimationController(const Ref<AnimationController>& contoller)
	{
		AnimContoller = contoller;
		View->SetAnimationController(contoller);
	}

	Ref<AnimationController>& MeshComponent::GetAnimationController()
	{
		return AnimContoller;
	}

	const Ref<Mesh>& MeshComponent::GetMesh() const
	{
		return Mesh;
	}

	const Ref<MeshView>& MeshComponent::GetMeshView() const
	{
		return View;
	}
}