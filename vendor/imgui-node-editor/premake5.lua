project "Node-Editor"
	kind "StaticLib"
	language "C++"
	outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

	targetdir ("../libs/" .. outputdir .. "/%{prj.name}")
	objdir ("../libs/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"src/**.h",
		"src/**.cpp",
		"src/**.inl"
	}

	includedirs
	{
		"src/",
		"../../smolengine.external/"
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


