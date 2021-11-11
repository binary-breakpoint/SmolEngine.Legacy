#include "stdafx.h"
#include "Graphics/Pools/MaterialPool.h"

namespace SmolEngine
{
	MaterialPool::MaterialPool()
	{

	}

	MaterialPool::~MaterialPool()
	{
		s_Instance = nullptr;
	}
}