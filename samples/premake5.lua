workspace "Frostium"
	architecture "x64"
	startproject "Samples"

	configurations
	{
		"Debug",
		"Release"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "Samples"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
	objdir ("../vendor/libs/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"PBR.h",
		"PBR.cpp",
	}

	includedirs
	{
		"../include/",
		"../include/Libraries",
		"../include/Libraries/spdlog/include",
		"../include/Libraries/cereal/include",
		"../include/Libraries/glm/",
	}

	links
	{
		"Frostium"
	}

	filter "system:windows"
		systemversion "latest"

		defines
		{
			"_CRT_SECURE_NO_WARNINGS",
			"PLATFORM_WIN"
		}


	--------------------------------------- Debug

	filter "configurations:Debug (Vulkan)"
	buildoptions "/MDd"
	buildoptions "/bigobj"
	symbols "on"

	filter "configurations:Debug (OpenGL)"
	buildoptions "/MDd"
	buildoptions "/bigobj"
	symbols "on"

	defines
	{
		"FROSTIUM_OPENGL_IMPL"
	}

	--------------------------------------- Release

	filter "configurations:Release (Vulkan)"
	buildoptions "/MD"
	buildoptions "/bigobj"
	optimize "on"

	filter "configurations:Release (OpenGL)"
	buildoptions "/MD"
	buildoptions "/bigobj"
	optimize "on"

	defines
	{
		"FROSTIUM_OPENGL_IMPL"
	}