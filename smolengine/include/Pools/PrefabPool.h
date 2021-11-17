#pragma once
#include "Memory.h"
#include "ECS/Prefab.h"

namespace SmolEngine
{
	class PrefabPool
	{
	public:
		static Ref<Prefab> ConstructFromPath(const std::string& path);
		static Ref<Prefab> GetByPath(const std::string& path);
	};
}