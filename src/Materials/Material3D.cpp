#include "stdafx.h"
#include "Materials/Material3D.h"

namespace SmolEngine
{
	bool Material3D::Build(MaterialCreateInfo* ci)
	{
		return BuildEX(ci, false);
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