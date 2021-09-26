#include "stdafx.h"
#include "Common/Profiler.h"

namespace SmolEngine
{
	void Profiler::Start()
	{
		m_Start = std::chrono::high_resolution_clock::now();
	}

	void Profiler::Finish()
	{
		auto stop = std::chrono::high_resolution_clock::now();
		auto time = std::chrono::duration_cast<std::chrono::milliseconds>(stop - m_Start);

		m_Time = static_cast<size_t>(time.count());
	}

	size_t Profiler::GetTimeInMilliseconds()
	{
		return m_Time;
	}
}