#include "stdafx.h"
#ifdef FROSTIUM_OPENGL_IMPL
#include "Backends/OpenGL/OpenglContext.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
    void OpenglContext::Setup(GLFWwindow* window)
    {
        LoadGL();
        m_Window = window;
        glfwMakeContextCurrent(m_Window);
    }

    void OpenglContext::LoadGL()
    {
        int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
        DebugLog::LogInfo("OpenGL info: \n\n              Version: {0}\n              Vendor: {1}\n              GPU: {2}\n", glGetString(GL_VERSION), glGetString(GL_VENDOR), glGetString(GL_RENDERER));
    }

    void OpenglContext::SwapBuffers()
    {
        glfwSwapBuffers(m_Window);
    }
}
#endif
