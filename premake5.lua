
workspace "Frostium"
	architecture "x64"
	startproject "Samples"

	configurations
	{
		"Debug (Vulkan)",
		"Release (Vulkan)",
		"Debug (OpenGL)",
		"Release (OpenGL)",
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Include directories relative to root folder (solution directory)
IncludeDir = {}
IncludeDir["glm"] = "include/Libraries/glm"
IncludeDir["ImGui"] = "include/Libraries/imgui"
IncludeDir["imgizmo"] = "include/Libraries/imgizmo/src"

IncludeDir["GLFW"] = "vendor/glfw/include"
IncludeDir["Glad"] = "vendor/glad/include"
IncludeDir["vulkan"] = "vendor/vulkan/include"
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
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

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
		"vendor/assimp/include",
		
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
		"GLFW",
		"Glad",
		"ImGui",
		"ImGizmo",
		"SPIRV-Cross",
		"KTX-Tools",
		
		"vendor/vulkan/libs/vulkan-1.lib",
		"vendor/vulkan/libs/VkLayer_utils.lib",
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

	filter "configurations:Debug (Vulkan)"
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
			"vendor/vulkan/libs/OSDependent_d.lib",

			"vendor/assimp/libs/assimp-vc142-mtd.lib",
			"vendor/assimp/libs/zlibstaticd.lib"
		}

		defines
		{
			"FROSTIUM_DEBUG"
		}

	filter "configurations:Debug (OpenGL)"
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
		"vendor/vulkan/libs/OSDependent_d.lib",

		"vendor/assimp/libs/assimp-vc142-mtd.lib",
		"vendor/assimp/libs/zlibstaticd.lib"
	}

	defines
	{
		"FROSTIUM_OPENGL_IMPL",
		"FROSTIUM_DEBUG"
	}

	filter "configurations:Release (Vulkan)"
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
			"vendor/vulkan/libs/OSDependent.lib",

			"vendor/assimp/libs/assimp-vc142-mt.lib",
			"vendor/assimp/libs/zlibstatic.lib"
		}

	filter "configurations:Release (OpenGL)"
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
		 "vendor/vulkan/libs/OSDependent.lib",

		 "vendor/assimp/libs/assimp-vc142-mt.lib",
		 "vendor/assimp/libs/zlibstatic.lib"
	   }

	    defines
	   {
		 "FROSTIUM_OPENGL_IMPL"
	   }



project "Samples"
	location "samples"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/**.h",
		"%{prj.name}/**.cpp",
	}

	includedirs
	{
		"include/",
		"include/Libraries",
		"include/Libraries/spdlog/include",
		"include/Libraries/cereal/include",
		"include/Libraries/glm/",
	}

	links
	{
		"Frostium"
	}

	filter "system:windows"
		systemversion "latest"

		defines
		{
			"_CRT_SECURE_NO_WARNINGS",
			"PLATFORM_WIN"
		}


	--------------------------------------- Debug

	filter "configurations:Debug (Vulkan)"
	buildoptions "/MDd"
	buildoptions "/bigobj"
	symbols "on"

	filter "configurations:Debug (OpenGL)"
	buildoptions "/MDd"
	buildoptions "/bigobj"
	symbols "on"

	defines
	{
		"FROSTIUM_OPENGL_IMPL"
	}

	--------------------------------------- Release

	filter "configurations:Release (Vulkan)"
	buildoptions "/MD"
	buildoptions "/bigobj"
	optimize "on"

	filter "configurations:Release (OpenGL)"
	buildoptions "/MD"
	buildoptions "/bigobj"
	optimize "on"

	defines
	{
		"FROSTIUM_OPENGL_IMPL"
	}