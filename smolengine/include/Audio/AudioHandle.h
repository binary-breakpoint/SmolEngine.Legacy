#pragma once
#include "Core/Core.h"

namespace SmolEngine
{
	struct AudioHandle
	{
		AudioHandle() = default;
		AudioHandle(uint32_t handle) : m_Handle(handle) {}
		operator uint32_t() const { return m_Handle; }

		uint32_t  GetID() const { return m_Handle; }

	private:
		uint32_t  m_Handle = 0;

		friend class AudioSystem;
		friend struct AudioSourceComponent;
	};
}