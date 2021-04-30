#pragma once

namespace Frostium
{
	class Mesh;

	struct AnimatorStorage
	{
		uint32_t  m_SkinIndex = 0;
		float     m_DeltaTime = 0.0f;
		Mesh*     m_Skins[1000];
	};

	class Animator
	{
	public:
		static void SubmitAnimaton(Mesh* mesh);
		static void SubmitAnimaton2D();

	private:
		static void BeginSubmit(float deltaTime);
		static void EndSubmit();

	private:
		static AnimatorStorage* s_Storage;

		friend class Renderer;
		friend class GraphicsContext;
	};
}