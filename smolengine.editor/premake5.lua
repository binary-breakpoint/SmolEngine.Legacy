project "SmolEngine-Editor"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"

	targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
	objdir ("../vendor/libs/bin-int/" .. outputdir .. "/%{prj.name}")
	linkoptions { "/ignore:4099" }

	VULKAN_SDK = os.getenv("VULKAN_SDK")

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
		"../smolengine.external/spdlog/include/",
		"../smolengine.external/taskflow/",
		"../smolengine.external/glm/",

		"../vendor/imgui-node-editor/src/",

		"%{VULKAN_SDK}/Include"
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
		symbols "on"

		postbuildcommands 
		{
			'{COPY} "../vendor/mono/bin/Debug/mono-2.0-sgen.dll" "%{cfg.targetdir}"',
		}
	
		filter "configurations:Release_Vulkan"
		optimize "on"

		postbuildcommands 
		{
			'{COPY} "../vendor/mono/bin/Release/mono-2.0-sgen.dll" "%{cfg.targetdir}"',
		}