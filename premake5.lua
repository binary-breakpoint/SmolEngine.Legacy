
workspace "SmolEngine.Graphics"
	architecture "x64"
	startproject "PBR"

	configurations
	{
		"Debug_Vulkan",
		"Release_Vulkan",
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["glm"] = "include/External/glm"
IncludeDir["ImGui"] = "include/External/imgui"
IncludeDir["imgizmo"] = "include/External/imgizmo/src"

IncludeDir["GLFW"] = "vendor/glfw/include"
IncludeDir["Glad"] = "vendor/glad/include"
IncludeDir["vulkan"] = "include/External/vulkan/include"
IncludeDir["stb"] = "include/External/stb_image"
IncludeDir["ktx"] = "vendor/ktx/include"
IncludeDir["gli"] = "vendor/gli"
IncludeDir["tinygltf"] = "vendor/tinygltf"
IncludeDir["cereal"] = "vendor/cereal/include"
IncludeDir["ozz"] = "vendor/ozz-animation/include"

group "Dependencies"
include "include/External/imgui"
include "vendor/glfw"
include "vendor/glad"
include "vendor/spir_v_cross"
include "vendor/ktx"
include "vendor/imgizmo"
group ""

project "SmolEngine.Graphics"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("vendor/libs/bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "stdafx.h"
	pchsource "src/stdafx.cpp"
	linkoptions { "/ignore:4006" }

	files
	{
		"include/**.h",
		"src/**.cpp",
		"src/**.h",

		"include/External/glm/glm/**.hpp",
		"include/External/glm/glm/**.inl",

		"include/External/stb_image/**.h",
		"vendor/stb_image/**.cpp",
		"vendor/implot/**.cpp",
		"vendor/vulkan_memory_allocator/vk_mem_alloc.cpp",
		"include/External/vulkan_memory_allocator/vk_mem_alloc.h",
	}

	includedirs
	{
		"vendor/",
		"vendor/glslang/include",
		"vendor/nvidia_aftermath/include",
		"vendor/icon_font_cpp_headers",

		"include/",
		"include/External",
		"include/External/implot/",
		"include/External/spdlog/include",

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
		"%{IncludeDir.cereal}",
		"%{IncludeDir.ozz}",
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
			"vendor/vulkan/libs/VkLayer_utils.lib",
			"vendor/vulkan/libs/SPIRV-Tools_d.lib",
			"vendor/vulkan/libs/SPIRV-Tools-opt_d.lib",
			"vendor/vulkan/libs/OGLCompiler_d.lib",
			"vendor/vulkan/libs/OSDependent_d.lib",

			"vendor/glslang/glslang-default-resource-limitsd.lib",
			"vendor/glslang/glslangd.lib",
			"vendor/glslang/GenericCodeGend.lib",
			"vendor/glslang/MachineIndependentd.lib",
			"vendor/glslang/SPVRemapperd.lib",
			"vendor/glslang/SPIRVd.lib",

			"vendor/nvidia_aftermath/lib/GFSDK_Aftermath_Lib.x64.lib",
			"vendor/nvidia_aftermath/lib/GFSDK_Aftermath_Lib_UWP.x64.lib",

			"vendor/ozz-animation/libs/ozz_animation_d.lib",
			"vendor/ozz-animation/libs/ozz_animation_offline_d.lib",
			"vendor/ozz-animation/libs/ozz_base_d.lib",
			"vendor/ozz-animation/libs/ozz_geometry_d.lib",
			"vendor/ozz-animation/libs/ozz_options_d.lib",
		}

		defines
		{
			"SMOLENGINE_DEBUG"
		}

		postbuildcommands
		{
			"{COPY} bin/" ..outputdir .. "/SmolEngine.Graphics/SmolEngine.Graphics.lib ../SmolEngine/vendor/frostium/libs/debug/",
			"{COPY} include ../SmolEngine/smolengine/include/Libraries/frostium/",
		}

	filter "configurations:Release_Vulkan"
	buildoptions "/MD"
	buildoptions "/bigobj"
	buildoptions "/Zm500"
	optimize "on"

		links 
		{ 

			"vendor/vulkan/libs/SPIRV-Tools.lib",
			"vendor/vulkan/libs/SPIRV-Tools-opt.lib",
			"vendor/vulkan/libs/OGLCompiler.lib",
			"vendor/vulkan/libs/OSDependent.lib",

			"vendor/glslang/glslang-default-resource-limits.lib",
			"vendor/glslang/glslang.lib",
			"vendor/glslang/GenericCodeGen.lib",
			"vendor/glslang/MachineIndependent.lib",
			"vendor/glslang/SPVRemapper.lib",
			"vendor/glslang/SPIRV.lib",

			"vendor/ozz-animation/libs/ozz_animation_r.lib",
			"vendor/ozz-animation/libs/ozz_animation_offline_r.lib",
			"vendor/ozz-animation/libs/ozz_base_r.lib",
			"vendor/ozz-animation/libs/ozz_geometry_r.lib",
			"vendor/ozz-animation/libs/ozz_options_r.lib",
		}

		postbuildcommands
		{
			"{COPY} bin/" ..outputdir .. "/SmolEngine.Graphics/SmolEngine.Graphics.lib ../SmolEngine/vendor/frostium/libs/release/",
			"{COPY} include ../SmolEngine/smolengine/include/Libraries/frostium/",
		}

--------------------------------------------------------------------------------- PBR

group "Tests"
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "PBR"
	kind "ConsoleApp"
	location "tests"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("vendor/libs/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"tests/PBR.h",
		"tests/PBR.cpp",
	}

	includedirs
	{
		"include/",
		
		"include/External",
		"include/External/vulkan/include",
		"include/External/spdlog/include",
		"include/External/glm/"
	}

	links
	{
		"bin/" ..outputdir .. "/SmolEngine.Graphics/SmolEngine.Graphics.lib"
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
		buildoptions "/bigobj"
		optimize "on"


------------------------------------------------- 2D

project "2D"
	kind "ConsoleApp"
	location "tests"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("vendor/libs/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"tests/2D.h",
		"tests/2D.cpp",
	}

	includedirs
	{
		"include/",
		
		"include/External",
		"include/External/vulkan/include",
		"include/External/spdlog/include",
		"include/External/glm/"
	}

	links
	{
		"bin/" ..outputdir .. "/SmolEngine.Graphics/SmolEngine.Graphics.lib"
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
		buildoptions "/bigobj"
		optimize "on"

	------------------------------------------------- 3D Animations

	project "Skinning"
	kind "ConsoleApp"
	location "tests"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("vendor/libs/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"tests/Skinning.h",
		"tests/Skinning.cpp",
	}

	includedirs
	{
		"include/",

		"include/External",
		"include/External/vulkan/include",
		"include/External/spdlog/include",
		"include/External/glm/"
	}

	links
	{
		"bin/" ..outputdir .. "/SmolEngine.Graphics/SmolEngine.Graphics.lib"
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
		buildoptions "/bigobj"
		optimize "on"

	------------------------------------------------- MATERIALS
	project "Materials"
	kind "ConsoleApp"
	location "tests"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("vendor/libs/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"tests/CustomMaterials.h",
		"tests/CustomMaterials.cpp",
	}

	includedirs
	{
		"include/",

		"include/External",
		"include/External/vulkan/include",
		"include/External/spdlog/include",
		"include/External/glm/"
	}

	links
	{
		"bin/" ..outputdir .. "/SmolEngine.Graphics/SmolEngine.Graphics.lib"
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
		buildoptions "/bigobj"
		optimize "on"

	------------------------------------------------- RAYTRACING
	project "Raytracing"
	kind "ConsoleApp"
	location "tests"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("vendor/libs/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"tests/Raytracing.h",
		"tests/Raytracing.cpp",
	}

	includedirs
	{
		"include/",

		"include/External",
		"include/External/vulkan/include",
		"include/External/spdlog/include",
		"include/External/glm/"
	}

	links
	{
		"bin/" ..outputdir .. "/SmolEngine.Graphics/SmolEngine.Graphics.lib"
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
		buildoptions "/bigobj"
		optimize "on"
		

	group ""