project "ImGizmo"
    kind "StaticLib"
    language "C"
	staticruntime "off"
    
    targetdir ("../libs/" .. outputdir .. "/%{prj.name}")
    objdir ("../libs/bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
		"src/**.cpp",
		"src/**.h",
    }

	includedirs
	{
		"../../smolengine.external/",
		"src/"
	}
    
    filter "system:windows"
        systemversion "latest"


		filter "configurations:Debug_Vulkan"
		symbols "on"
		
		filter "configurations:Release_Vulkan"
		optimize "full"

	

