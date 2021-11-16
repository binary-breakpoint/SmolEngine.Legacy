#include "stdafx.h"
#include "Core/Engine.h"

#include "ECS/Systems/ScriptingSystem.h"
#include "ECS/Components/Singletons/GraphicsEngineSComponent.h"

namespace SmolEngine 
{
	Engine* Engine::s_Instance = nullptr;

	Engine::Engine()
	{
		if (s_Instance != nullptr)
		{
			abort();
		}

		s_Instance = this;
	}

	Engine::~Engine()
	{
		DebugLog::LogInfo("State = Shutdown");
		m_Running = false;
	}

	void Engine::Init()
	{
		DebugLog::LogInfo("State = Startup");
		//---------------------------------------------------------------------///

		CreateGraphicsModule();
		CreateEngineModule();
		CreateScriptModule();

		OnInitializationComplete(m_World);

		DebugLog::LogInfo("Initialized successfully");
		m_Running = true;
		Run();
	}

	void Engine::Shutdown()
	{
		if (m_Running)
		{
			DebugLog::LogInfo("State = Shutdown");

			m_Running = false;
			m_GraphicsContext->Shutdown();

			exit(EXIT_SUCCESS);
		}
	}

	void Engine::Run()
	{
		DebugLog::LogInfo("State = Runtime");

		while (m_Running)
		{
			float deltaTime = m_GraphicsContext->CalculateDeltaTime();
			m_GraphicsContext->ProcessEvents();

			if (m_GraphicsContext->IsWindowMinimized())
				continue;

			m_GraphicsContext->BeginFrame(deltaTime);
			{
				for (Layer* layer : *m_LayerHandler)
					layer->OnBeginFrame(deltaTime);

				m_World->OnBeginFrame();
				{
					for (Layer* layer : *m_LayerHandler)
					{
						layer->OnUpdate(deltaTime);
						layer->OnImGuiRender();
					}

					m_World->OnUpdate(deltaTime);

					for (Layer* layer : *m_LayerHandler)
						layer->OnEndFrame(deltaTime);
				}
				m_World->OnEndFrame();
			}
			m_GraphicsContext->SwapBuffers();
		}
	}

	void Engine::OnEvent(Event& e)
	{
		if (e.IsType(EventType::WINDOW_CLOSE))
			OnWindowClose(e);

		for (auto result = m_LayerHandler->end(); result != m_LayerHandler->begin();)
		{
			(*--result)->OnEvent(e);
			if (e.m_Handled)
				break;
		}
	}

	void Engine::OnWindowClose(Event& e)
	{
		Shutdown();
	}

	void Engine::SetOnSceneLoadedCallback(const std::function<void(Scene*)>& callback)
	{

	}

	void Engine::SetOnSceneUnLoadedCallback(const std::function<void(Scene*)>& callback)
	{

	}

	void Engine::CreateGraphicsModule()
	{
		WindowCreateInfo windoInfo = {};
		{
			windoInfo.bFullscreen = false;
			windoInfo.bVSync = false;
			windoInfo.Height = 480;
			windoInfo.Width = 720;
			windoInfo.Title = "SmolEngine";
		}

		GraphicsContextCreateInfo info = {};
		{
			info.ResourcesFolder = "../resources/";
			info.pWindowCI = &windoInfo;
		}

		OnGraphicsModuleCreation(&info);

		m_GraphicsContext = GraphicsContext::Create(&info);
		m_GraphicsContext->SetEventCallback(std::bind_front(&Engine::OnEvent, this));
	}

	void Engine::CreatePhysicsModule()
	{
		PhysicsModuleCreateInfo physicsContextCI = {};
		OnPhysicsModuleCreation(&physicsContextCI);
	}

	void Engine::CreateEngineModule()
	{
		m_World = new WorldAdmin();

		EngineModuleCreateInfo engineContextCI = {};
		OnEngineModuleCreation(&engineContextCI);
		m_AssetsFolder = engineContextCI.AssetsFolder;

		m_LayerHandler = new LayerManager();
		OnLayerModuleCreation(m_LayerHandler);
	}

	void Engine::CreateScriptModule()
	{
		Scope<ScriptingSystem> system = std::make_unique<ScriptingSystem>();
		OnScriptModuleCreation(system.get());
	}

	std::string Engine::GetAssetsFolder() const
	{
		return m_AssetsFolder;
	}
}
