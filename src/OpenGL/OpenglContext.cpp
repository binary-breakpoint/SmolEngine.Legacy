#include "stdafx.h"
#include "OpenGL/OpenglContext.h"
#include "Common/SLog.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

namespace Frostium 
{
    void OpenglContext::Setup(GLFWwindow* window)
    {
        m_Window = window;

        glfwMakeContextCurrent(m_Window);
        int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
        NATIVE_INFO("OpenGL info: \n\n              Version: {0}\n              Vendor: {1}\n              GPU: {2}\n", glGetString(GL_VERSION), glGetString(GL_VENDOR), glGetString(GL_RENDERER));
    }

    void OpenglContext::SwapBuffers()
    {
        glfwSwapBuffers(m_Window);
    }
}
