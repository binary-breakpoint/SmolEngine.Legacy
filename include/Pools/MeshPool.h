#pragma once

#include "Common/Memory.h"
#include "Primitives/Mesh.h"

#include <unordered_map>
#include <mutex>

namespace SmolEngine
{
	class MeshPool
	{
	public:
		MeshPool(const std::string& root);
		~MeshPool();

		static Ref<Mesh>                      GetCube();
		static Ref<Mesh>                      GetSphere();
		static Ref<Mesh>                      GetCapsule();
		static Ref<Mesh>                      GetTorus();
								              
		static Ref<Mesh>                      GetByPath(const std::string& path);
		static Ref<Mesh>                      GetByID(size_t id);
											
		static Ref<Mesh>                      ConstructFromFile(const std::string& path);
		static bool                           Remove(size_t id);
		static void                           Clear();
											  
	private:								  
		inline static MeshPool*               s_Instance = nullptr;
		Ref<Mesh>                             m_Cube = nullptr;
		Ref<Mesh>                             m_Sphere = nullptr;
		Ref<Mesh>                             m_Capsule = nullptr;
		Ref<Mesh>                             m_Torus = nullptr;
		std::mutex                            m_Mutex{};
		std::unordered_map<size_t, Ref<Mesh>> m_IDMap;

		friend class GraphicsContext;
	};
}