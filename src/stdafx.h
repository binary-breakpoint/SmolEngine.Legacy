#pragma once

#ifdef PLATFORM_WIN
#include <Windows.h>
#endif

#include <memory>
#include <iostream>
#include <algorithm>
#include <functional>
#include <thread>
#include <utility>
#include <fstream>
#include <sstream>
#include <filesystem>

#include <vector>
#include <array>
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <string>

#include "GraphicsContext.h"
#include "Common/DebugLog.h"

#ifndef OPENGL_IMPL
#include "Vulkan/Vulkan.h"
#endif

// MSVC ENABLE/DISABLE WARNING DEFINITION
#	define VKBP_DISABLE_WARNINGS() \
		__pragma(warning(push, 0))

#	define VKBP_ENABLE_WARNINGS() \
		__pragma(warning(pop))


