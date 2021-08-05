#include "stdafx.h"
#include "Tools/DefaultMeshes.h"

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	DefaultMeshes::DefaultMeshes(const std::string& root)
	{
		Cube = std::make_shared<Mesh>();
		Sphere = std::make_shared<Mesh>();
		Capsule = std::make_shared<Mesh>();
		Torus = std::make_shared<Mesh>();

		Mesh::Create(root + "Models/cube.gltf", Cube.get());
		Mesh::Create(root + "Models/sphere.gltf", Sphere.get());
		Mesh::Create(root + "Models/capsule.gltf", Capsule.get());
		Mesh::Create(root + "Models/torus.gltf", Torus.get());
	}
}