#pragma once
#include "Core/Core.h"

#include <glm/glm.hpp>

namespace SmolEngine
{
	struct DebugDrawState;
	struct WorldAdminStateSComponent;
	struct GraphicsEngineSComponent;

	class RendererSystem
	{
	private:
		static void OnRender();
		static void OnUpdate();
		static void OnDebugDraw();

		static void SubmitLights();
		static void SubmitMeshes();
		static void SubmitSprites();

		static int  GetLayerIndex(int index);

	private:

		inline static WorldAdminStateSComponent* m_World = nullptr;
		inline static GraphicsEngineSComponent*  m_State = nullptr;

		friend class EditorLayer;
		friend class WorldAdmin;
		friend class GameView;

	};
}