project "Glad"
    kind "StaticLib"
    language "C"
    staticruntime "on"
    
    targetdir ("../libs/" .. outputdir .. "/%{prj.name}")
    objdir ("../libs/bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "include/glad/glad.h",
        "include/KHR/khrplatform.h",
        "src/glad.c"
    }

	includedirs
	{
		"include"
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
	optimize "on"

	filter "configurations:Release_OpenGL"
	buildoptions "/MD"
	optimize "on"

