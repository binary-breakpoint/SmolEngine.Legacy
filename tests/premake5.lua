project "PBR"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"

	targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
	objdir ("../vendor/libs/bin-int/" .. outputdir .. "/%{prj.name}")

	VULKAN_SDK = os.getenv("VULKAN_SDK")

	files
	{
		"PBR.h",
		"PBR.cpp",
	}

	includedirs
	{
		"../smolengine.core/include/",
		"../smolengine.graphics/include/",

		"../smolengine.external/",
		"../smolengine.external/spdlog/include",
		"../smolengine.external/glm/",

		"%{VULKAN_SDK}/Include"
	}

	links
	{
		"SmolEngine.Graphics"
	}

	postbuildcommands
	{
		"{COPY} ../vendor/nvidia_aftermath/lib/copy ../bin/" .. outputdir .. "/%{prj.name}",
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
	
		filter "configurations:Release_Vulkan"
		optimize "on"


------------------------------------------------- 2D

project "2D"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"

	targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
	objdir ("../vendor/libs/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"2D.h",
		"2D.cpp",
	}

	includedirs
	{
		"../smolengine.core/include/",
		"../smolengine.graphics/include/",

		"../smolengine.external/",
		"../smolengine.external/spdlog/include",
		"../smolengine.external/glm/",

		"%{VULKAN_SDK}/Include"
	}

	links
	{
		"SmolEngine.Graphics"
	}

	postbuildcommands
	{
		"{COPY} ../vendor/nvidia_aftermath/lib/copy ../bin/" .. outputdir .. "/%{prj.name}",
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
	
		filter "configurations:Release_Vulkan"
		optimize "on"

	------------------------------------------------- 3D Animations

	project "Skinning"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"

	targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
	objdir ("../vendor/libs/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"Skinning.h",
		"Skinning.cpp",
	}

	includedirs
	{
		"../smolengine.core/include/",
		"../smolengine.graphics/include/",

		"../smolengine.external/",
		"../smolengine.external/spdlog/include",
		"../smolengine.external/glm/",

		"%{VULKAN_SDK}/Include"
	}

	links
	{
		"SmolEngine.Graphics"
	}

	postbuildcommands
	{
		"{COPY} ../vendor/nvidia_aftermath/lib/copy ../bin/" .. outputdir .. "/%{prj.name}",
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
	
		filter "configurations:Release_Vulkan"
		optimize "on"

	------------------------------------------------- MATERIALS
	project "Materials"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"

	targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
	objdir ("../vendor/libs/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"CustomMaterials.h",
		"CustomMaterials.cpp",
	}

	includedirs
	{
		"../smolengine.core/include/",
		"../smolengine.graphics/include/",

		"../smolengine.external/",
		"../smolengine.external/spdlog/include",
		"../smolengine.external/glm/",

		"%{VULKAN_SDK}/Include"
	}

	links
	{
		"SmolEngine.Graphics"
	}

	postbuildcommands
	{
		"{COPY} ../vendor/nvidia_aftermath/lib/copy ../bin/" .. outputdir .. "/%{prj.name}",
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
	
		filter "configurations:Release_Vulkan"
		optimize "on"

	------------------------------------------------- RAYTRACING

	project "Raytracing"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"

	targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
	objdir ("../vendor/libs/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"Raytracing.h",
		"Raytracing.cpp",
	}

	includedirs
	{
		"../smolengine.core/include/",
		"../smolengine.graphics/include/",

		"../smolengine.external/",
		"../smolengine.external/spdlog/include",
		"../smolengine.external/glm/",

		"%{VULKAN_SDK}/Include"
	}

	links
	{
		"SmolEngine.Graphics"
	}

	postbuildcommands
	{
		"{COPY} ../vendor/nvidia_aftermath/lib/copy ../bin/" .. outputdir .. "/%{prj.name}",
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
	
		filter "configurations:Release_Vulkan"
		optimize "on"
		
