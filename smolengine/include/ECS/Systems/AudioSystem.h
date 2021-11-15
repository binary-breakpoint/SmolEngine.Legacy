#pragma once
#include "Core/Core.h"
#include "Audio/AudioClip.h"
#include "Audio/AudioHandle.h"

#include <glm/glm.hpp>
#include <string>

namespace SmolEngine
{
	enum class AudioFilterFlags : int;

	struct WorldAdminStateSComponent;
	struct AudioSourceComponent;
	struct AudioEngineSComponent;
	struct AudioSource;

	class AudioSystem
	{
	public:
		static void      StopAll();
		static float*    GetFFT();
		static float*    GetWave();
		static void      SetGlobalVolume(float value);

	private:
		static void      OnBeginWorld();
		static void      OnEndWorld();
		static void      RemoveHandle(uint32_t handle);
		static void      OnUpdate(const glm::vec3 camPos);

	private:
		inline static AudioEngineSComponent*     m_State = nullptr;
		inline static WorldAdminStateSComponent* m_World = nullptr;

		friend class EditorLayer;
		friend class GameView;
		friend class WorldAdmin;
		friend struct AudioSourceComponent;
	};
}