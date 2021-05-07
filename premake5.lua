
workspace "Frostium"
	architecture "x64"
	startproject "PBR"

	configurations
	{
		"Debug_Vulkan",
		"Release_Vulkan",

		"Debug_OpenGL",
		"Release_OpenGL",
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Include directories relative to root folder (solution directory)
IncludeDir = {}
IncludeDir["glm"] = "include/Libraries/glm"
IncludeDir["ImGui"] = "include/Libraries/imgui"
IncludeDir["imgizmo"] = "include/Libraries/imgizmo/src"

IncludeDir["GLFW"] = "vendor/glfw/include"
IncludeDir["Glad"] = "vendor/glad/include"
IncludeDir["vulkan"] = "include/Libraries/vulkan/include"
IncludeDir["stb"] = "vendor/stb_image"
IncludeDir["ktx"] = "vendor/ktx/include"
IncludeDir["gli"] = "vendor/gli"
IncludeDir["tinygltf"] = "vendor/tinygltf"

group "Dependencies"
include "include/Libraries/imgui"
include "vendor/glfw"
include "vendor/glad"
include "vendor/spir_v_cross"
include "vendor/ktx"
include "vendor/imgizmo"
group ""


project "Frostium"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("vendor/libs/bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "stdafx.h"
	pchsource "src/stdafx.cpp"

	files
	{
		"include/**.h",
		"src/**.cpp",
		"src/**.h",

		"include/Libraries/glm/glm/**.hpp",
		"include/Libraries/glm/glm/**.inl",

		"vendor/stb_image/**.h",
		"vendor/stb_image/**.cpp",
		"vendor/implot/**.cpp",
	}

	includedirs
	{
		"vendor/",
		"vendor/icon_font_cpp_headers",
		
		"include/",
		"include/Libraries",
		"include/Libraries/implot/",
		"include/Libraries/spdlog/include",
		"include/Libraries/cereal/include",

		"src/",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.stb}",
        "%{IncludeDir.entt}",
		"%{IncludeDir.vulkan}",
		"%{IncludeDir.imgizmo}",
		"%{IncludeDir.ktx}",
		"%{IncludeDir.meta}",
		"%{IncludeDir.gli}",
		"%{IncludeDir.tinygltf}",
		"%{IncludeDir.taskflow}",
	}

	links 
	{ 
		"vendor/libs/" ..outputdir .. "/Glad/Glad.lib",
		"vendor/libs/" ..outputdir .. "/GLFW/GLFW.lib",
		"vendor/libs/" ..outputdir .. "/ImGizmo/ImGizmo.lib",
		"vendor/libs/" ..outputdir .. "/ImGui/ImGui.lib",
		"vendor/libs/" ..outputdir .. "/KTX-Tools/KTX-Tools.lib",
		"vendor/libs/" ..outputdir .. "/SPIRV-Cross/SPIRV-Cross.lib",

		"vendor/vulkan/libs/vulkan-1.lib",
		"vendor/vulkan/libs/VkLayer_utils.lib"
	}

	defines
	{
        "_CRT_SECURE_NO_WARNINGS",
		"GLFW_INCLUDE_NONE"
	}

	filter "system:windows"
		staticruntime "On"
		systemversion "latest"

		defines
		{
			"PLATFORM_WIN",
			"BUILD_DLL",
			"GLFW_INCLUDE_NONE"
		}

	filter "configurations:Debug_Vulkan"
		buildoptions "/MDd"
		buildoptions "/bigobj"
		buildoptions "/Zm500"
		symbols "on"

		links 
		{ 
			"vendor/vulkan/libs/shaderc_d.lib",
			"vendor/vulkan/libs/shaderc_util_d.lib",
			"vendor/vulkan/libs/glslang_d.lib",
			"vendor/vulkan/libs/SPIRV_d.lib",
			"vendor/vulkan/libs/SPIRV-Tools_d.lib",
			"vendor/vulkan/libs/SPIRV-Tools-opt_d.lib",
			"vendor/vulkan/libs/machineIndependent_d.lib",
			"vendor/vulkan/libs/genericCodeGen_d.lib",
			"vendor/vulkan/libs/OGLCompiler_d.lib",
			"vendor/vulkan/libs/OSDependent_d.lib"
		}

		defines
		{
			"FROSTIUM_DEBUG"
		}

	filter "configurations:Debug_OpenGL"
	buildoptions "/MDd"
	buildoptions "/bigobj"
	buildoptions "/Zm500"
	symbols "on"

	links 
	{ 
		"opengl32.lib",
		"vendor/vulkan/libs/shaderc_d.lib",
		"vendor/vulkan/libs/shaderc_util_d.lib",
		"vendor/vulkan/libs/glslang_d.lib",
		"vendor/vulkan/libs/SPIRV_d.lib",
		"vendor/vulkan/libs/SPIRV-Tools_d.lib",
		"vendor/vulkan/libs/SPIRV-Tools-opt_d.lib",
		"vendor/vulkan/libs/machineIndependent_d.lib",
		"vendor/vulkan/libs/genericCodeGen_d.lib",
		"vendor/vulkan/libs/OGLCompiler_d.lib",
		"vendor/vulkan/libs/OSDependent_d.lib"
	}

	defines
	{
		"FROSTIUM_OPENGL_IMPL",
		"FROSTIUM_DEBUG"
	}

	filter "configurations:Release_Vulkan"
		buildoptions "/MD"
		buildoptions "/bigobj"
		buildoptions "/Zm500"
		optimize "on"

		links 
		{ 
			"vendor/vulkan/libs/shaderc.lib",
			"vendor/vulkan/libs/shaderc_util.lib",
			"vendor/vulkan/libs/glslang.lib",
			"vendor/vulkan/libs/SPIRV.lib",
			"vendor/vulkan/libs/SPIRV-Tools.lib",
			"vendor/vulkan/libs/SPIRV-Tools-opt.lib",
			"vendor/vulkan/libs/machineIndependent.lib",
			"vendor/vulkan/libs/genericCodeGen.lib",
			"vendor/vulkan/libs/OGLCompiler.lib",
			"vendor/vulkan/libs/OSDependent.lib"
		}

	filter "configurations:Release_OpenGL"
	buildoptions "/MD"
	buildoptions "/bigobj"
	buildoptions "/Zm500"
	optimize "on"

	   links 
	   { 
		"opengl32.lib",
		 "vendor/vulkan/libs/shaderc.lib",
		 "vendor/vulkan/libs/shaderc_util.lib",
		 "vendor/vulkan/libs/glslang.lib",
		 "vendor/vulkan/libs/SPIRV.lib",
		 "vendor/vulkan/libs/SPIRV-Tools.lib",
		 "vendor/vulkan/libs/SPIRV-Tools-opt.lib",
		 "vendor/vulkan/libs/machineIndependent.lib",
		 "vendor/vulkan/libs/genericCodeGen.lib",
		 "vendor/vulkan/libs/OGLCompiler.lib",
		 "vendor/vulkan/libs/OSDependent.lib"
	   }

	    defines
	   {
		 "FROSTIUM_OPENGL_IMPL"
	   }

--------------------------------------------------------------------------------- PBR

group "Samples"
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "PBR"
	kind "ConsoleApp"
	location "samples"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("vendor/libs/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"samples/PBR.h",
		"samples/PBR.cpp",
	}

	includedirs
	{
		"include/",
		"include/Libraries",
		"include/Libraries/spdlog/include",
		"include/Libraries/cereal/include",
		"include/Libraries/glm/"
	}

	links
	{
		"bin/" ..outputdir .. "/Frostium/Frostium.lib"
	}

	filter "system:windows"
		systemversion "latest"

		defines
		{
			"_CRT_SECURE_NO_WARNINGS",
			"PLATFORM_WIN"
		}

	filter "configurations:Debug_Vulkan"
	buildoptions "/MDd"
	buildoptions "/bigobj"
	symbols "on"

	filter "configurations:Release_Vulkan"
		buildoptions "/MD"
		optimize "on"

--------------------------------------------------------- C#

project "CSharpBinding"
	kind "ConsoleApp"
	location "samples"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("vendor/libs/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"samples/CSharpBinding.h",
		"samples/CSharpBinding.cpp",
	}

	includedirs
	{
		"include/",
		"include/Libraries",
		"include/Libraries/spdlog/include",
		"include/Libraries/cereal/include",
		"include/Libraries/glm/",
		"vendor/mono/include/mono-2.0"
	}

	links
	{
		"bin/" ..outputdir .. "/Frostium/Frostium.lib",
		"vendor/mono/lib/mono-2.0-sgen.lib"
	}

	filter "system:windows"
		systemversion "latest"

		defines
		{
			"_CRT_SECURE_NO_WARNINGS",
			"PLATFORM_WIN"
		}

	filter "configurations:Debug_Vulkan"
	buildoptions "/MDd"
	buildoptions "/bigobj"
	symbols "on"

	filter "configurations:Release_Vulkan"
		buildoptions "/MD"
		optimize "on"

------------------------------------------------- 2D

project "2D"
	kind "ConsoleApp"
	location "samples"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("vendor/libs/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"samples/2D.h",
		"samples/2D.cpp",
	}

	includedirs
	{
		"include/",
		"include/Libraries",
		"include/Libraries/spdlog/include",
		"include/Libraries/cereal/include",
		"include/Libraries/glm/"
	}

	links
	{
		"bin/" ..outputdir .. "/Frostium/Frostium.lib"
	}

	filter "system:windows"
		systemversion "latest"

		defines
		{
			"_CRT_SECURE_NO_WARNINGS",
			"PLATFORM_WIN"
		}

	filter "configurations:Debug_Vulkan"
	buildoptions "/MDd"
	buildoptions "/bigobj"
	symbols "on"

	filter "configurations:Release_Vulkan"
		buildoptions "/MD"
		optimize "on"

	------------------------------------------------- 3D Animations

	project "Skinning"
	kind "ConsoleApp"
	location "samples"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("vendor/libs/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"samples/Skinning.h",
		"samples/Skinning.cpp",
	}

	includedirs
	{
		"include/",
		"include/Libraries",
		"include/Libraries/spdlog/include",
		"include/Libraries/cereal/include",
		"include/Libraries/glm/"
	}

	links
	{
		"bin/" ..outputdir .. "/Frostium/Frostium.lib"
	}

	filter "system:windows"
		systemversion "latest"

		defines
		{
			"_CRT_SECURE_NO_WARNINGS",
			"PLATFORM_WIN"
		}

	filter "configurations:Debug_Vulkan"
	buildoptions "/MDd"
	buildoptions "/bigobj"
	symbols "on"

	filter "configurations:Release_Vulkan"
		buildoptions "/MD"
		optimize "on"

	group ""