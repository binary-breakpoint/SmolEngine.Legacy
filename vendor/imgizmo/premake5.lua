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

	--------------------------------------- Debug

	filter "configurations:Debug (Vulkan)"
	buildoptions "/MDd"
	symbols "on"

	filter "configurations:Debug (OpenGL)"
	buildoptions "/MDd"
	symbols "on"

	--------------------------------------- Release

	filter "configurations:Release (Vulkan)"
	buildoptions "/MD"
	optimize "on"

	filter "configurations:Release (OpenGL)"
	buildoptions "/MD"
	optimize "on"

