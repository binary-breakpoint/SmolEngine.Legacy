
workspace "SmolEngine"
	architecture "x64"
	startproject "PBR"

	configurations
	{
		"Debug_Vulkan",
		"Release_Vulkan",
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

group "Dependencies"
include "vendor/glfw"
include "vendor/glad"
include "vendor/spir_v_cross"
include "vendor/ktx"
include "vendor/imgizmo"
include "smolengine.external/imgui"
group ""

group "Engine"
include "smolengine.core"
include "smolengine.graphics"
group ""

group "Tests"
include "tests"
group ""