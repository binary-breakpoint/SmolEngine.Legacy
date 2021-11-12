project "SmolEngine"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
	objdir ("../vendor/libs/bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "stdafx.h"
	pchsource "smolengine/src/stdafx.cpp"

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

		"../vendor/mono/include/mono-2.0/",
		"../vendor/bullet3/include/",
		"../vendor/soloud/include/",
		"../vendor/cereal/include/",
		"../vendor/imgui-node-editor/src/",

		"../smolengine.external/",
		"../smolengine.external/box_2D/include/",
		"../smolengine.external/vulkan/include",
		"../smolengine.external/spdlog/include/",
		"../smolengine.external/taskflow/",
		"../smolengine.external/glm/",
	}

	links 
	{ 
		"../vendor/mono/lib/mono-2.0-sgen.lib",
		"../vendor/libs/" ..outputdir .. "/Box2D/Box2D.lib",
		"../vendor/libs/" ..outputdir .. "/Soloud/Soloud.lib",
		"../vendor/libs/" ..outputdir .. "/Node-Editor/Node-Editor.lib",
		"../bin/" ..outputdir .. "/SmolEngine.Graphics/SmolEngine.Graphics.lib"
	}

	defines
	{
        "_CRT_SECURE_NO_WARNINGS",
		"GLFW_INCLUDE_NONE",
	}

	filter "system:windows"
		staticruntime "On"
		systemversion "latest"

		defines
		{
			"PLATFORM_WIN",
			"BUILD_DLL",
			"GLFW_INCLUDE_NONE"
		}

	filter "configurations:Debug"
		buildoptions "/MDd"
		buildoptions "/bigobj"
		buildoptions "/Zm500"
		symbols "on"

		links 
		{ 
		   "../vendor/bullet3/libs/BulletCollision_Debug.lib",
		   "../vendor/bullet3/libs/BulletDynamics_Debug.lib",
		   "../vendor/bullet3/libs/LinearMath_Debug.lib"
		}

		defines
		{
			"SMOLENGINE_DEBUG",
			"_DEBUG"
		}
		

	filter "configurations:Release"
		buildoptions "/MD"
		buildoptions "/bigobj"
		buildoptions "/Zm500"
		optimize "on"

		links 
		{ 
		   "../vendor/bullet3/libs/BulletCollision.lib",
		   "../vendor/bullet3/libs/BulletDynamics.lib",
		   "../vendor/bullet3/libs/LinearMath.lib"
		}

		defines
		{
			"NDEBUG"
		}
