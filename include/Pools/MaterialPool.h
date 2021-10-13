#pragma once
#include "Common/Memory.h"

namespace SmolEngine
{
	class MaterialPool
	{
	public:		
		MaterialPool();
		~MaterialPool();

	private:
		inline static MaterialPool*           s_Instance = nullptr;
	};
}