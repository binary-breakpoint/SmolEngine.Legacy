#include "stdafx.h"
#include "Materials/Material2D.h"

namespace SmolEngine
{
	bool Material2D::Build(MaterialCreateInfo* ci)
	{
		return BuildEX(ci, true);
	}
}