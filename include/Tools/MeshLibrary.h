#pragma once

#include "Common/Common.h"
#include "Primitives/Mesh.h"

namespace SmolEngine
{
	class MeshLibrary
	{
	public:
		MeshLibrary(const std::string& root);

		Ref<Mesh> Cube = nullptr;
		Ref<Mesh> Sphere = nullptr;
		Ref<Mesh> Capsule = nullptr;
		Ref<Mesh> Torus = nullptr;
	};
}