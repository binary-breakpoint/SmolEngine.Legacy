#pragma once
#include "GraphicsContext.h"

#include "Core/Core.h"
#include "ECS/Scene.h"
#include "ECS/AssetManager.h"

#include <entt/entity/registry.hpp>
#include <unordered_map>
#include <string>

namespace SmolEngine
{
	class Mesh;

	struct WorldAdminStateSComponent
	{
		WorldAdminStateSComponent();
		~WorldAdminStateSComponent();
		// Dummy c-tors - required by EnTT
		WorldAdminStateSComponent(WorldAdminStateSComponent& another) {}
		WorldAdminStateSComponent(WorldAdminStateSComponent&& other) {}
		WorldAdminStateSComponent& operator=(WorldAdminStateSComponent other) { return *this; }

		static WorldAdminStateSComponent* GetSingleton() { return s_Instance; }
														    
		inline static WorldAdminStateSComponent*              s_Instance = nullptr;
		bool                                                  m_InPlayMode = false;
		bool                                                  m_LevelEditorActive = false;
		entt::registry*                                       m_CurrentRegistry = nullptr;
		Scene*                                                m_ActiveScene = nullptr;
		AssetManager                                          m_AssetManager{};
		Scene                                                 m_Scenes[2];
	};
}