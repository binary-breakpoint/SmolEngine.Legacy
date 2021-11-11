#pragma once
#include "Core/Memory.h"
#include "Graphics/Primitives/Mesh.h"

#include <unordered_map>
#include <mutex>

namespace SmolEngine
{
	class MeshPool
	{
	public:
		MeshPool(const std::string& root);
		~MeshPool();

		static std::pair<Ref<Mesh>, Ref<MeshView>> GetCube();
		static std::pair<Ref<Mesh>, Ref<MeshView>> GetSphere();
		static std::pair<Ref<Mesh>, Ref<MeshView>> GetCapsule();
		static std::pair<Ref<Mesh>, Ref<MeshView>> GetTorus();	              
		static std::pair<Ref<Mesh>, Ref<MeshView>> GetByPath(const std::string& path);
		static std::pair<Ref<Mesh>, Ref<MeshView>> GetByID(size_t id);							
		static std::pair<Ref<Mesh>, Ref<MeshView>> ConstructFromFile(const std::string& path);

		static bool Remove(size_t id);
		static void Clear();
											  
	private:								  
		Ref<Mesh> m_Cube = nullptr;
		Ref<Mesh> m_Sphere = nullptr;
		Ref<Mesh> m_Capsule = nullptr;
		Ref<Mesh> m_Torus = nullptr;
		std::mutex m_Mutex{};
		std::unordered_map<size_t, Ref<Mesh>> m_IDMap;
		inline static MeshPool* s_Instance = nullptr;

		friend class GraphicsContext;
	};
}