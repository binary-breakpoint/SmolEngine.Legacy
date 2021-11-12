#pragma once
#include "Core/Core.h"

#include <vector>

namespace SoLoud
{
	class Soloud;
}

namespace SmolEngine
{
	struct AudioHandle;

	struct AudioEngineSComponent
	{
		AudioEngineSComponent();
		~AudioEngineSComponent();

		SoLoud::Soloud*                Core = nullptr;
		std::vector<Ref<AudioHandle>>  Handles;
		std::vector<uint32_t>          RemoveList;

		static AudioEngineSComponent* Get() { return Instance; }

	private:

		static AudioEngineSComponent* Instance;
	};
}