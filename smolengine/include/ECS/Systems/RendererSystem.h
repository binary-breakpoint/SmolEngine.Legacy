#pragma once
#include "Core/Core.h"

namespace SmolEngine
{
	enum class MeshTypeEX : uint32_t;

	struct DebugDrawState;
	struct WorldAdminStateSComponent;
	struct GraphicsEngineSComponent;
	struct Texture2DComponent;
	struct MeshComponent;

	class RendererSystem
	{
		static int  GetLayerIndex(int index);

		static void OnRender();
		static void OnUpdate();
		static void OnDebugDraw();
		static void SubmitLights();
		static void SubmitMeshes();
		static void SubmitSprites();
	private:

		inline static WorldAdminStateSComponent* m_World = nullptr;
		inline static GraphicsEngineSComponent*  m_State = nullptr;

		friend class EditorLayer;
		friend class WorldAdmin;
		friend class GameView;

	};
}