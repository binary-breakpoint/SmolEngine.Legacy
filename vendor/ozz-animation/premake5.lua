project "Ozz-Animation"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"
	outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

	targetdir ("../libs/" .. outputdir .. "/%{prj.name}")
	objdir ("../libs/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"src/**.cc",
		"include/**.h",
	}

	includedirs
	{
		"src/",
		"include/",
		"extern/"
	}

	filter "system:windows"
		systemversion "latest"

		filter "configurations:Release_Vulkan"
		optimize "on"

		filter "configurations:Debug_Vulkan"
		symbols "on"


