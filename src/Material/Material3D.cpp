#include "stdafx.h"
#include "Material/Material3D.h"

namespace SmolEngine
{
	void Material3D::SetDrawCallback(const std::function<void(CommandBuffer*, Material3D*)>& callback)
	{
		m_DrawCallback = callback;
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