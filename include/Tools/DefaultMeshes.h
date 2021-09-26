#pragma once

#include "Common/Common.h"
#include "Primitives/Mesh.h"

namespace SmolEngine
{
	class DefaultMeshes
	{
	public:
		DefaultMeshes(const std::string& root);

		Ref<Mesh> Cube = nullptr;
		Ref<Mesh> Sphere = nullptr;
		Ref<Mesh> Capsule = nullptr;
		Ref<Mesh> Torus = nullptr;

	private:
		friend class GraphicsContext;
	};
}