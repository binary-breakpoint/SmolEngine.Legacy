#include "stdafx.h"
#include "Pools/MeshPool.h"

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
		std::hash<std::string_view> hasher{};
		size_t id = hasher(path);

		return GetByID(id);
	}

	std::pair<Ref<Mesh>, Ref<MeshView>> MeshPool::GetByID(size_t id)
	{
		auto& it = s_Instance->m_IDMap.find(id);
		if (it != s_Instance->m_IDMap.end())
		{
			return { it->second, it->second->CreateMeshView() };
		}

		return { nullptr, nullptr };
	}

	std::pair<Ref<Mesh>, Ref<MeshView>> MeshPool::ConstructFromFile(const std::string& path)
	{
		size_t id = 0;
		{
			std::hash<std::string_view> hasher{};
			size_t id = hasher(path);

			auto& [mesh, view] = GetByID(id);
			if (view && mesh) { return { mesh, view }; }
		}

		Ref<Mesh> mesh = Mesh::Create();
		bool is_loaded = mesh->LoadFromFile(path);
		if (is_loaded)
		{
			s_Instance->m_Mutex.lock();
			s_Instance->m_IDMap[id] = mesh;
			s_Instance->m_Mutex.unlock();

			return { mesh, mesh->CreateMeshView() };
		}

		return { nullptr, nullptr };
	}

	bool MeshPool::Remove(size_t id)
	{
		return s_Instance->m_IDMap.erase(id);
	}

	void MeshPool::Clear()
	{
		s_Instance->m_IDMap.clear();
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