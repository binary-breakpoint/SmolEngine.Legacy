#include "stdafx.h"
#include "Graphics/Materials/Material2D.h"

namespace SmolEngine
{
	bool Material2D::Build(MaterialCreateInfo* ci)
	{
		return BuildEX(ci, true);
	}
}