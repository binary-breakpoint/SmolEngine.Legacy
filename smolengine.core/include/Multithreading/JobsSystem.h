#pragma once
#include <taskflow/taskflow/include/taskflow.hpp>

namespace SmolEngine
{
	class JobsSystem
	{
	public:
		JobsSystem();

		static void             BeginSubmition();
		static void             EndSubmition(bool wait = true);

		static uint32_t         GetNumWorkers();
		static uint32_t         GetNumTasks();
		static tf::Executor*    GetExecutor();
		static bool             GetActive();

		template<typename... F>
		static void             Schedule(F&&... f) { s_Instance->m_Queue.emplace(std::forward<F>(f)...); }

	private:
		bool                    m_IsActive = false;
		static JobsSystem*      s_Instance;
		tf::Taskflow            m_Queue{};
		tf::Executor            m_Executor{};
	};
}