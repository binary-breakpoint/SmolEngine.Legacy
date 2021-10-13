#include "stdafx.h"
#include "Pools/MaterialPool.h"
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