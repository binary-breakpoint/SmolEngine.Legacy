#include "stdafx.h"
#include "Materials/Material3D.h"
#include "Renderer/RendererShared.h"

namespace SmolEngine
{
	bool Material3D::Build(MaterialCreateInfo* ci)
	{
		return BuildEX(ci, false);
	}

	void Material3D::OnPushConstant(const uint32_t& dataOffset)
	{
		SubmitPushConstant(ShaderType::Vertex, sizeof(uint32_t), &dataOffset);
	}

	void Material3D::OnDrawCommand(Ref<Mesh>& mesh, DrawPackage* command)
	{
		DrawMeshIndexed(mesh, command->Instances);
	}

	VertexInputInfo Material3D::GetVertexInputInfo() const
	{
		BufferLayout layout =
		{
			{ DataTypes::Float3, "aPos" },
			{ DataTypes::Float3, "aNormal" },
			{ DataTypes::Float3, "aTangent" },
			{ DataTypes::Float2, "aUV" },
			{ DataTypes::Int4,   "aBoneIDs"},
			{ DataTypes::Float4, "aWeight"}
		};

		return VertexInputInfo(sizeof(PBRVertex), layout);
	}
}