project "SPIRV-Cross"
	kind "StaticLib"
	language "C++"

	targetdir ("../libs/" .. outputdir .. "/%{prj.name}")
	objdir ("../libs/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"src/**.h",
		"src/**.hpp",
		"src/**.cpp",
	}

	includedirs
	{
		"../../include/Libraries/spir_v_cross/include"
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
		optimize "full"
	
		filter "configurations:Release_OpenGL"
		buildoptions "/MD"
		optimize "full"
	
		
		filter "configurations:SmolEngine_R"
		buildoptions "/MD"
		optimize "full"
	
		filter "configurations:SmolEngine_D"
		buildoptions "/MDd"
		symbols "on"

