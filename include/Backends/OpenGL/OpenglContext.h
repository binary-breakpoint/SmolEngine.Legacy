#pragma once
#ifdef FROSTIUM_OPENGL_IMPL
struct GLFWwindow;

namespace SmolEngine
{
	class OpenglContext
	{
	public:

		void Setup(GLFWwindow* window);
		void LoadGL();
		void SwapBuffers();

		// Getters
		inline GLFWwindow* GetWindow() { return m_Window; }

	private:

		GLFWwindow* m_Window = nullptr;
	};
}
#endif