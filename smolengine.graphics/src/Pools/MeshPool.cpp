#include "stdafx.h"
#include "Pools/MeshPool.h"
#include "Asset/AssetManager.h"

namespace SmolEngine
{
	MeshPool::MeshPool(const std::string& root)
	{
		s_Instance = this;

		m_Cube = Mesh::Create();
		m_Cube->LoadFromFile(root + "Models/cube.gltf");

		m_Sphere = Mesh::Create();
		m_Sphere->LoadFromFile(root + "Models/sphere.gltf");

		m_Capsule = Mesh::Create();
		m_Capsule->LoadFromFile(root + "Models/capsule.gltf");

		m_Torus = Mesh::Create();
		m_Torus->LoadFromFile(root + "Models/torus.gltf");
	}

	MeshPool::~MeshPool()
	{
		s_Instance = nullptr;
	}

	std::pair<Ref<Mesh>, Ref<MeshView>> MeshPool::GetByPath(const std::string& path)
	{
		Ref<Mesh> mesh = AssetManager::GetAsset<Mesh>(path);
		if (mesh) { return { mesh, mesh->CreateMeshView() }; }

		return { nullptr, nullptr };
	}

	std::pair<Ref<Mesh>, Ref<MeshView>> MeshPool::ConstructFromFile(const std::string& path)
	{
		auto& pair = GetByPath(path);
		if (pair.first && pair.second)
			return pair;

		Ref<Mesh> mesh = Mesh::Create();
		bool is_loaded = mesh->LoadFromFile(path);
		if (is_loaded)
		{
			AssetManager::Add(path, mesh, AssetType::Mesh);
			return { mesh, mesh->CreateMeshView() };
		}

		return { nullptr, nullptr };
	}

	std::pair<Ref<Mesh>, Ref<MeshView>> MeshPool::GetCube()
	{
		return { s_Instance->m_Cube, s_Instance->m_Cube->CreateMeshView() };
	}

	std::pair<Ref<Mesh>, Ref<MeshView>> MeshPool::GetSphere()
	{
		return { s_Instance->m_Sphere, s_Instance->m_Sphere->CreateMeshView() };
	}

	std::pair<Ref<Mesh>, Ref<MeshView>> MeshPool::GetCapsule()
	{
		return { s_Instance->m_Capsule, s_Instance->m_Capsule->CreateMeshView() };
	}

	std::pair<Ref<Mesh>, Ref<MeshView>> MeshPool::GetTorus()
	{
		return { s_Instance->m_Torus, s_Instance->m_Torus->CreateMeshView() };
	}
}