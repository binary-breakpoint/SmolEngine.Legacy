#pragma once
#include "Core/Memory.h"

namespace SmolEngine
{
	class Texture;

	class PBRLoader
	{
	public:
		static Ref<PBRLoader> Create();

		virtual void  Free() = 0;
		virtual void  GeneratePBRCubeMaps(Ref<Texture>& environment_map) = 0;
		virtual void* GetBRDFLUTDesriptor() { return nullptr; }
		virtual void* GetIrradianceDesriptor() { return nullptr; }
		virtual void* GetPrefilteredCubeDesriptor() { return nullptr; }
	};
}