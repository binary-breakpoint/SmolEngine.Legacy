project "SmolEngine"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"

	targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
	objdir ("../vendor/libs/bin-int/" .. outputdir .. "/%{prj.name}")
	linkoptions { "/ignore:4006" }
	pchheader "stdafx.h"
	pchsource "src/stdafx.cpp"

	VULKAN_SDK = os.getenv("VULKAN_SDK")

	files
	{
		"include/**.h",
		"src/**.cpp"
	}

	includedirs
	{
		"include/",

		"../smolengine.core/include/",
		"../smolengine.graphics/include/",

		"../vendor/bullet3/include/",
		"../vendor/soloud/include/",
		"../vendor/cereal/include/",
		"../vendor/mono/include/",
		"../vendor/imgui-node-editor/src/",

		"../smolengine.external/",
		"../smolengine.external/box_2D/include/",
		"../smolengine.external/spdlog/include/",
		"../smolengine.external/taskflow/",
		"../smolengine.external/glm/",

		"%{VULKAN_SDK}/Include",
	}

	links 
	{ 
		"Box2D",
		"Soloud",
		"Node-Editor",
		"SmolEngine.Graphics"
	}

	defines
	{
        "_CRT_SECURE_NO_WARNINGS",
		"GLFW_INCLUDE_NONE",
	}

	filter "system:windows"
		systemversion "latest"

		defines
		{
			"PLATFORM_WIN",
			"BUILD_DLL",
			"GLFW_INCLUDE_NONE"
		}

		filter "configurations:Debug_Vulkan"
		symbols "on"

		links 
		{ 
			"../vendor/mono/lib/Debug/mono-2.0-sgen.lib",

		   "../vendor/bullet3/libs/BulletCollision_Debug.lib",
		   "../vendor/bullet3/libs/BulletDynamics_Debug.lib",
		   "../vendor/bullet3/libs/LinearMath_Debug.lib"
		}

		defines
		{
			"SMOLENGINE_DEBUG",
			"_DEBUG"
		}
		

		filter "configurations:Release_Vulkan"
		optimize "on"

		links 
		{ 
			"../vendor/mono/lib/Release/mono-2.0-sgen.lib",

		   "../vendor/bullet3/libs/BulletCollision.lib",
		   "../vendor/bullet3/libs/BulletDynamics.lib",
		   "../vendor/bullet3/libs/LinearMath.lib"
		}

		defines
		{
			"NDEBUG"
		}
