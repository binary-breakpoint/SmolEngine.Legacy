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
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <string>

#ifndef FROSTIUM_OPENGL_IMPL
#include "Vulkan/Vulkan.h"
#endif


