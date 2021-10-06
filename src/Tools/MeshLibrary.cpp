#include "stdafx.h"
#include "Tools/MeshLibrary.h"

namespace SmolEngine
{
	MeshLibrary::MeshLibrary(const std::string& root)
	{
		Cube = Mesh::Create();
		Cube->LoadFromFile(root + "Models/cube.gltf");

		Sphere = Mesh::Create();
		Sphere->LoadFromFile(root + "Models/sphere.gltf");

		Capsule = Mesh::Create();
		Capsule->LoadFromFile(root + "Models/capsule.gltf");

		Torus = Mesh::Create();
		Torus->LoadFromFile(root + "Models/torus.gltf");
	}
}