
project "KTX-Tools"
	kind "StaticLib"
	language "C++"
	outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

	targetdir ("../libs/" .. outputdir .. "/%{prj.name}")
	objdir ("../libs/bin-int/" .. outputdir .. "/%{prj.name}")

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
		"../../include/Libraries/vulkan/include/",
		"lib/"
	}

	filter "system:windows"
		systemversion "latest"
		staticruntime "on"

		defines 
		{ 
			"_CRT_SECURE_NO_WARNINGS"
		}


		filter "configurations:Debug_Vulkan"
		buildoptions "/MDd"
		symbols "on"
	
		filter "configurations:Debug_OpenGL"
		buildoptions "/MDd"
		symbols "on"
	
		filter "configurations:Release_Vulkan"
		buildoptions "/MD"
		optimize "on"
	
		filter "configurations:Release_OpenGL"
		buildoptions "/MD"
		optimize "on"
	

