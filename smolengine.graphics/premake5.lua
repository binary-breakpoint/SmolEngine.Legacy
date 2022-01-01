project "SmolEngine.Graphics"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"

	targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
	objdir ("../vendor/libs/bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "stdafx.h"
	pchsource "src/stdafx.cpp"
	linkoptions { "/ignore:4006" }

	VULKAN_SDK = os.getenv("VULKAN_SDK")

	files
	{
		"src/**.cpp",
		"src/**.h",

		"include/**.h",
		"include/GraphicsCore.h",

		"../smolengine.external/stb_image/**.h",
		"../smolengine.external/vulkan_memory_allocator/vk_mem_alloc.h",

		"../vendor/implot/**.cpp",
		"../vendor/vulkan_memory_allocator/vk_mem_alloc.cpp",
	}

	includedirs
	{
		"include/",
		"src/",

		"../smolengine.external/",
		"../smolengine.external/implot/",
		"../smolengine.external/spdlog/include",
		"../smolengine.external/glm",
		"../smolengine.external/imgui",
		"../smolengine.external/imgizmo/src",
		"../smolengine.external/stb_image",
		"../smolengine.core/include/",

		"../vendor/",
		"../vendor/nvidia_aftermath/include",
		"../vendor/icon_font_cpp_headers",
		"../vendor/ozz-animation/include",
		"../vendor/cereal/include",
		"../vendor/glfw/include",
		"../vendor/glad/include",
		"../vendor/ktx/include",
		"../vendor/tinygltf",
		"../vendor/gli",

		"%{VULKAN_SDK}/Include",
		"%{VULKAN_SDK}/Include/glslang/include",
	}

	links 
	{ 
		"Glad",
		"GLFW",
		"ImGizmo",
		"ImGui",
		"KTX-Tools",
		"Ozz-Animation",
		"SmolEngine.Core",

		"%{VULKAN_SDK}/Lib/vulkan-1.lib",
	}

	defines
	{
        "_CRT_SECURE_NO_WARNINGS",
		"GLFW_INCLUDE_NONE"
	}

	filter "system:windows"
		systemversion "latest"

		defines
		{
			"PLATFORM_WIN",
			"BUILD_DLL",
			"GLFW_INCLUDE_NONE"
		}

	filter "configurations:Debug_Vulkan"
		symbols "on"

		links 
		{ 
			"%{VULKAN_SDK}/Lib/VkLayer_utils.lib",

			"%{VULKAN_SDK}/Lib/spirv-cross-glsld.lib",
			"%{VULKAN_SDK}/Lib/spirv-cross-cored.lib",

			"%{VULKAN_SDK}/Lib/OGLCompilerd.lib",
			"%{VULKAN_SDK}/Lib/OSDependentd.lib",
			"%{VULKAN_SDK}/Lib/SPIRV-Toolsd.lib",
			"%{VULKAN_SDK}/Lib/SPIRV-Tools-optd.lib",

			"%{VULKAN_SDK}/Lib/glslang-default-resource-limitsd.lib",
			"%{VULKAN_SDK}/Lib/glslangd.lib",
			"%{VULKAN_SDK}/Lib/GenericCodeGend.lib",
			"%{VULKAN_SDK}/Lib/MachineIndependentd.lib",
			"%{VULKAN_SDK}/Lib/SPVRemapperd.lib",
			"%{VULKAN_SDK}/Lib/SPIRVd.lib",

			"../vendor/nvidia_aftermath/lib/GFSDK_Aftermath_Lib.x64.lib",
			"../vendor/nvidia_aftermath/lib/GFSDK_Aftermath_Lib_UWP.x64.lib",
		}

		defines
		{
			"SMOLENGINE_DEBUG"
		}

	filter "configurations:Release_Vulkan"
	optimize "on"

		links 
		{ 
			"%{VULKAN_SDK}/Lib/spirv-cross-glsl.lib",
			"%{VULKAN_SDK}/Lib/spirv-cross-core.lib",
			"%{VULKAN_SDK}/Lib/SPIRV-Tools.lib",
			"%{VULKAN_SDK}/Lib/SPIRV-Tools-opt.lib",
			"%{VULKAN_SDK}/Lib/OGLCompiler.lib",
			"%{VULKAN_SDK}/Lib/OSDependent.lib",

			"%{VULKAN_SDK}/Lib/glslang-default-resource-limits.lib",
			"%{VULKAN_SDK}/Lib/glslang.lib",
			"%{VULKAN_SDK}/Lib/GenericCodeGen.lib",
			"%{VULKAN_SDK}/Lib/MachineIndependent.lib",
			"%{VULKAN_SDK}/Lib/SPVRemapper.lib",
			"%{VULKAN_SDK}/Lib/SPIRV.lib"
		}
