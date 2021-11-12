#pragma once
#include "Core/Core.h"

#include <glm/glm.hpp>
#include <unordered_map>
#include <entt/entt.hpp>

#include "Window/Events.h"

namespace SmolEngine
{ 
	struct WorldAdminStateSComponent;
	struct CameraComponent;
	struct TransformComponent;
	struct SystemInstance;
	struct SceneData;
	struct MeshComponent;
	struct MaterialCreateInfo;
	struct SceneViewProjection;
	class Scene;
	class Actor;
	class AudioEngine;
	class Mesh;
	class Texture;
	class AssetManager;

	class WorldAdmin
	{
	public:
		WorldAdmin();

		bool                         SaveCurrentScene();
		bool                         LoadLastSceneState();
		bool                         LoadSceneRuntime(const std::string& path);
		bool                         CreateScene(const std::string& filePath);
		bool                         SaveScene(const std::string& filePath);
		bool                         LoadScene(const std::string& filePath, bool reload = false);
		bool                         SwapScene(uint32_t index);
		bool                         IsInPlayMode();
								     
		inline static WorldAdmin*    GetSingleton() { return s_World; }
		Scene*                       GetActiveScene();
		AssetManager*                GetAssetManager();

	private:                  
		void                         OnEndWorld();
		void                         OnBeginWorld();
		void                         OnBeginFrame();
		void                         OnEndFrame();
		void                         OnWorldReset();
		void                         OnUpdate(float deltaTime);
		void                         OnEvent(Event& e);
		void                         OnHotReload(); 
		void                         ReloadMaterials();
		bool                         LoadStaticComponents();
		bool                         ChangeActorName(Ref<Actor>& actor, const std::string& name);

	private:
		inline static WorldAdmin*    s_World = nullptr;
		WorldAdminStateSComponent*   m_State = nullptr;
		entt::registry               m_GlobalRegistry{};
		entt::entity                 m_GlobalEntity{};

	private:

		friend class Animation2DController;
		friend class SettingsWindow;
		friend class AnimationPanel;
		friend class EditorLayer;
		friend class Actor;
		friend class SceneView;
		friend class Engine;
	};
}