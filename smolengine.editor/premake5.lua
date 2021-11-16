project "SmolEngine-Editor"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
	objdir ("../vendor/libs/bin-int/" .. outputdir .. "/%{prj.name}")
	linkoptions { "/ignore:4099" }

	files
	{
		"include/**.h",
		"src/**.cpp",
	}

	includedirs
	{
		"include/",
		"src/",

		"../smolengine/include/",
		"../smolengine.core/include",
		"../smolengine.graphics/include",

		"../smolengine.external/",
		"../smolengine.external/box_2D/include/",
		"../smolengine.external/vulkan/include/",
		"../smolengine.external/spdlog/include/",
		"../smolengine.external/taskflow/",
		"../smolengine.external/glm/",

		"../vendor/imgui-node-editor/src/"
	}

	links
	{
		"SmolEngine"
	}

	filter "system:windows"
		systemversion "latest"

		defines
		{
			"_CRT_SECURE_NO_WARNINGS",
			"PLATFORM_WIN"
		}

		filter "configurations:Debug_Vulkan"
		buildoptions "/MDd"
		buildoptions "/bigobj"
		symbols "on"
	
		filter "configurations:Release_Vulkan"
		buildoptions "/MD"
		buildoptions "/bigobj"
		optimize "on"