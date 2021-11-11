#include "stdafx.h"

#ifdef OPENGL_IMPL
#define IMGUI_IMPL_OPENGL_LOADER_GLAD
#include <imgui/examples/imgui_impl_glfw.cpp>
#include <imgui/examples/imgui_impl_opengl3.cpp>
#else
#define GLFW_INCLUDE_VULKAN
#include <imgui/examples/imgui_impl_glfw.cpp>
#include <imgui/examples/imgui_impl_vulkan.cpp>
#endif

#include <imgui/misc/cpp/imgui_stdlib.cpp>