#pragma once

#include "Common/Core.h"
#include "Primitives/Mesh.h"

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
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