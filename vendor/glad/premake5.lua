project "Glad"
    kind "StaticLib"
    language "C"
	staticruntime "off"
    
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
	symbols "on"

	filter "configurations:Release_Vulkan"
	optimize "full"

