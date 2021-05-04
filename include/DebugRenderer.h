#pragma once
#include "Common/Core.h"
#include "Common/RendererShared.h"

#include "Utils/GLM.h"

namespace Frostium
{
	class Mesh;

	struct DebugRendererProperties
	{
		glm::vec4  LineColor;
		float      LineWidth;
	};

	class DebugRenderer
	{
	public:

		static void BeginDebug();
		static void EndDebug();

		static void DrawQuad(const glm::vec2& pos, const glm::vec2& rotation, const glm::vec2& scale);
		static void DrawCirlce(const glm::vec2& pos, const glm::vec2& scale);
		static void DrawMesh(const glm::vec3& pos, const glm::vec3& rotation, const glm::vec3& scale, Mesh* mesh);

	private:

		static void Init();

		friend class GraphicsContext;
		friend class Renderer;
	};
}