#pragma once
#include "Graphics/Materials/Material.h"
#include "Graphics/Common/Vertex.h"

namespace SmolEngine
{
	struct DrawPackage;

	class Material3D : public Material
	{
	public:
		Material3D() = default;
		virtual ~Material3D() = default;

		bool         Build(MaterialCreateInfo* ci);

		virtual void OnPushConstant(const uint32_t& dataOffset);
		virtual void OnDrawCommand(Ref<Mesh>& mesh, DrawPackage* command);

		VertexInputInfo GetVertexInputInfo() const;
	};
}