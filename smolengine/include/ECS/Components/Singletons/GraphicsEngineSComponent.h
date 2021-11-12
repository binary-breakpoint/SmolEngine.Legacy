#pragma once

#include "GraphicsContext.h"
#include "Renderer/RendererDebug.h"
#include "Renderer/RendererDeferred.h"
#include "Renderer/Renderer2D.h"

namespace SmolEngine
{
	enum class DebugDrawFlags
	{
		Disabled,
		Default,
		Bullet,
		Wireframes
	};

	struct GraphicsEngineSComponent
	{
		GraphicsEngineSComponent();
		~GraphicsEngineSComponent();
		// Dummy c-tors - required by EnTT
		GraphicsEngineSComponent(GraphicsEngineSComponent& another) {}
		GraphicsEngineSComponent(GraphicsEngineSComponent&& other) {}
		GraphicsEngineSComponent& operator=(GraphicsEngineSComponent other) { return *this; }
		
		SceneViewProjection    ViewProj;
		DebugDrawFlags         eDebugDrawFlags = DebugDrawFlags::Default;

		static GraphicsEngineSComponent* Get() { return Instance; }

	private:

		inline static GraphicsEngineSComponent* Instance = nullptr;
	};
}