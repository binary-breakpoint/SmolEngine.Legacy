#include "stdafx.h"
#ifdef FROSTIUM_SMOLENGINE_IMPL
#include "Multithreading/JobsSystemInstance.h"

namespace SmolEngine
{
	JobsSystemInstance* JobsSystemInstance::s_Instance = nullptr;

	JobsSystemInstance::JobsSystemInstance()
	{
		s_Instance = this;
	}

	void JobsSystemInstance::BeginSubmition()
	{
		s_Instance->m_RenderingQueue.clear();
	}

	void JobsSystemInstance::EndSubmition(bool wait)
	{
		auto& queue = s_Instance->m_RenderingQueue;
		auto& executor = s_Instance->m_Executor;

		executor.run(queue);

		if (wait)
		{
			executor.wait_for_all();
		}
	}

	uint32_t JobsSystemInstance::GetNumWorkers()
	{
		return static_cast<uint32_t>(s_Instance->m_Executor.num_workers());
	}

	uint32_t JobsSystemInstance::GetNumRenderingTasks()
	{
		return static_cast<uint32_t>(s_Instance->m_RenderingQueue.num_tasks());
	}

	tf::Executor* JobsSystemInstance::GetExecutor()
	{
		return &s_Instance->m_Executor;
	}
}
#endif