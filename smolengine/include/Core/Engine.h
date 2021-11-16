#pragma once
#include "Core.h"
#include "Layer/LayerManager.h"

#ifndef FROSTIUM_SMOLENGINE_IMPL
#define FROSTIUM_SMOLENGINE_IMPL
#endif

#include "Window/Events.h"

#include <memory>
#include <functional>
#include <glm/glm.hpp>

namespace SmolEngine
{
	struct WorldAdminStateSComponent;
	struct GraphicsContextCreateInfo;

	class WorldAdmin;
	class Scene;
	class ScriptingSystem;
	class GraphicsContext;

	struct PhysicsModuleCreateInfo
	{
		glm::vec3 Gravity = glm::vec3(0.0f, -9.81f, 0.0f);
	};

	struct EngineModuleCreateInfo
	{
		std::string AssetsFolder = "../tests/";
	};

	class Engine
	{
	public:
		virtual ~Engine();
		Engine();

		void                         Init();
		void                         Shutdown();
									 
		// Methods to implement		 
		virtual void                 OnEngineModuleCreation(EngineModuleCreateInfo* info) {}
		virtual void                 OnGraphicsModuleCreation(GraphicsContextCreateInfo* info) {}
		virtual void                 OnPhysicsModuleCreation(PhysicsModuleCreateInfo* info) {}
		virtual void                 OnLayerModuleCreation(LayerManager* layerManager) {}
		virtual void                 OnScriptModuleCreation(ScriptingSystem* system) {}
		virtual void                 OnInitializationComplete(WorldAdmin* admin) {}
									 
		// Getters					 
		inline static Engine*        GetEngine() { return s_Instance; }
		LayerManager*                GetLayerManager() const { return m_LayerHandler; }
		std::string                  GetAssetsFolder() const;
									 
		// Callbacks				 
		void                         SetOnSceneLoadedCallback(const std::function<void(Scene*)>& callback);
		void                         SetOnSceneUnLoadedCallback(const std::function<void(Scene*)>& callback);
									 
	private:
		void                         CreatePhysicsModule();
		void                         CreateEngineModule();
		void                         CreateScriptModule();
		void                         CreateGraphicsModule();

		void                         Run();
		void                         OnWindowClose(Event& e);
		void                         OnEvent(Event& event);

	private:
		static Engine*               s_Instance;
		bool                         m_Running = false;
		WorldAdmin*                  m_World = nullptr;
		LayerManager*                m_LayerHandler = nullptr;
		Ref<GraphicsContext>         m_GraphicsContext = nullptr;
		std::string                  m_AssetsFolder;
		std::function<void(Scene*)>  m_SceneLoadCl = nullptr;
		std::function<void(Scene*)>  m_SceneUnLoadCl = nullptr;
	};
}

SmolEngine::Engine* CreateEngineContext();

