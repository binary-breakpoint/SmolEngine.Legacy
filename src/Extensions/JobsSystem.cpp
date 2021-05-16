#include "stdafx.h"
#ifdef FROSTIUM_SMOLENGINE_IMPL
#include "Extensions/JobsSystem.h"

namespace Frostium
{
	JobsSystem* JobsSystem::s_Instance = nullptr;

	JobsSystem::JobsSystem()
	{
		s_Instance = this;
	}

	void JobsSystem::BeginSubmition()
	{
		s_Instance->m_RenderingQueue.clear();
	}

	void JobsSystem::EndSubmition(bool wait)
	{
		auto& queue = s_Instance->m_RenderingQueue;
		auto& executor = s_Instance->m_Executor;

		executor.run(queue);

		if (wait)
		{
			executor.wait_for_all();
		}
	}

	uint32_t JobsSystem::GetNumWorkers()
	{
		return static_cast<uint32_t>(s_Instance->m_Executor.num_workers());
	}

	uint32_t JobsSystem::GetNumRenderingTasks()
	{
		return static_cast<uint32_t>(s_Instance->m_RenderingQueue.num_tasks());
	}
}
#endif