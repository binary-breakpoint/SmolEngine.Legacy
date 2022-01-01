
project "KTX-Tools"
	kind "StaticLib"
	language "C++"
	outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

	targetdir ("../libs/" .. outputdir .. "/%{prj.name}")
	objdir ("../libs/bin-int/" .. outputdir .. "/%{prj.name}")

	VULKAN_SDK = os.getenv("VULKAN_SDK")

	files
	{
		"lib/**.h",
		"lib/**.c",
		"lib/**.cxx",
		"lib/**.inl",
	}

	includedirs
	{
		"include/",
		"other_include/",
		"lib/",

		"%{VULKAN_SDK}/Include",
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
	

