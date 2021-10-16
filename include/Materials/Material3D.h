#pragma once
#include "Materials/Material.h"
#include "Common/Vertex.h"

namespace SmolEngine
{
	struct DrawPackage;

	class Material3D : public Material
	{
	public:
		Material3D() = default;
		virtual ~Material3D() = default;

		bool         Build(MaterialCreateInfo* ci);
		virtual void OnPushConstant(const uint32_t& dataOffset) = 0;
		virtual void OnDrawCommand(Ref<Mesh>& mesh, DrawPackage* command) = 0;

		VertexInputInfo GetVertexInputInfo() const;
	};
}