#include "Multithreading/JobsSystem.h"

namespace SmolEngine
{
	JobsSystem* JobsSystem::s_Instance = nullptr;

	JobsSystem::JobsSystem()
	{
		s_Instance = this;
	}

	void JobsSystem::BeginSubmition()
	{
		s_Instance->m_IsActive = true;
		s_Instance->m_Queue.clear();
	}

	void JobsSystem::EndSubmition(bool wait)
	{
		auto& queue = s_Instance->m_Queue;
		auto& executor = s_Instance->m_Executor;

		executor.run(queue);

		if (wait)
		{
			executor.wait_for_all();
		}

		s_Instance->m_IsActive = false;
	}

	uint32_t JobsSystem::GetNumWorkers()
	{
		return static_cast<uint32_t>(s_Instance->m_Executor.num_workers());
	}

	uint32_t JobsSystem::GetNumTasks()
	{
		return static_cast<uint32_t>(s_Instance->m_Queue.num_tasks());
	}

	tf::Executor* JobsSystem::GetExecutor()
	{
		return &s_Instance->m_Executor;
	}

	bool JobsSystem::GetActive()
	{
		return s_Instance->m_IsActive;
	}
}