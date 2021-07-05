#include "stdafx.h"
#include "Common/DefaultMeshes.h"

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	DefaultMeshes::~DefaultMeshes()
	{
		if (Cube != nullptr)
		{
			delete Cube, Sphere, Capsule, Torus;

			Cube = nullptr;
			Sphere = nullptr;
			Capsule = nullptr;
			Torus = nullptr;
		}
	}

	void DefaultMeshes::Init(const std::string& root)
	{
		Cube = new Mesh();
		Sphere = new Mesh();
		Capsule = new Mesh();
		Torus = new Mesh();

		Mesh::Create(root + "Models/cube.gltf", Cube);
		Mesh::Create(root + "Models/sphere.gltf", Sphere);
		Mesh::Create(root + "Models/capsule.gltf", Capsule);
		Mesh::Create(root + "Models/torus.gltf", Torus);
	}
}