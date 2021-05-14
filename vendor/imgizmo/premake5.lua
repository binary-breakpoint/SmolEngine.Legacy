project "ImGizmo"
    kind "StaticLib"
    language "C"
    staticruntime "on"
    
    targetdir ("../libs/" .. outputdir .. "/%{prj.name}")
    objdir ("../libs/bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
		"src/**.cpp",
		"src/**.h",
    }

	includedirs
	{
		"../../include/Libraries/",
		"src/"
	}
    
    filter "system:windows"
        systemversion "latest"


		filter "configurations:Debug_Vulkan"
		buildoptions "/MDd"
		symbols "on"
	
		filter "configurations:Debug_OpenGL"
		buildoptions "/MDd"
		symbols "on"
	
		filter "configurations:Release_Vulkan"
		buildoptions "/MD"
		optimize "full"
	
		filter "configurations:Release_OpenGL"
		buildoptions "/MD"
		optimize "full"
	

