project "Soloud"
	kind "StaticLib"
	language "C++"
	outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

	targetdir ("../libs/" .. outputdir .. "/%{prj.name}")
	objdir ("../libs/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"src/**.cpp",
		"src/**.h",
		"src/**.c",
		"include/**.h",
		"include/**.c",
	}

	includedirs
	{
		"include/",
	}

	defines
	{
		"WITH_MINIAUDIO"
	}

	filter "system:windows"
		systemversion "latest"
		staticruntime "off"

		defines 
		{ 
			"_CRT_SECURE_NO_WARNINGS"
		}

		filter "configurations:Debug_Vulkan"
		symbols "on"
	
		filter "configurations:Release_Vulkan"
		optimize "full"


