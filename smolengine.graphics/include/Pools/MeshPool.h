#pragma once
#include "Memory.h"
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

		static std::pair<Ref<Mesh>, Ref<MeshView>> GetCube();
		static std::pair<Ref<Mesh>, Ref<MeshView>> GetSphere();
		static std::pair<Ref<Mesh>, Ref<MeshView>> GetCapsule();
		static std::pair<Ref<Mesh>, Ref<MeshView>> GetTorus();	              
		static std::pair<Ref<Mesh>, Ref<MeshView>> GetByPath(const std::string& path);					
		static std::pair<Ref<Mesh>, Ref<MeshView>> ConstructFromFile(const std::string& path);
											  
	private:								  
		Ref<Mesh> m_Cube = nullptr;
		Ref<Mesh> m_Sphere = nullptr;
		Ref<Mesh> m_Capsule = nullptr;
		Ref<Mesh> m_Torus = nullptr;

		inline static MeshPool* s_Instance = nullptr;
	};
}