#pragma once
#include "Materials/Material.h"

namespace SmolEngine
{
	class Material2D: public Material
	{
	public:
		bool Build(MaterialCreateInfo* ci);
	};
}