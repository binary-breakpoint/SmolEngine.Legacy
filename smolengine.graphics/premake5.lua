project "SmolEngine.Graphics"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
	objdir ("../vendor/libs/bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "stdafx.h"
	pchsource "src/stdafx.cpp"
	linkoptions { "/ignore:4006" }

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
		"../smolengine.external/vulkan/include",
		"../smolengine.external/stb_image",
		"../smolengine.core/include/",

		"../vendor/",
		"../vendor/glslang/include",
		"../vendor/nvidia_aftermath/include",
		"../vendor/icon_font_cpp_headers",
		"../vendor/ozz-animation/include",
		"../vendor/cereal/include",
		"../vendor/glfw/include",
		"../vendor/glad/include",
		"../vendor/ktx/include",
		"../vendor/tinygltf",
		"../vendor/gli",
	}

	links 
	{ 
		"../bin/" ..outputdir .. "/SmolEngine.Core/SmolEngine.Core.lib",

		"../vendor/libs/" ..outputdir .. "/Glad/Glad.lib",
		"../vendor/libs/" ..outputdir .. "/GLFW/GLFW.lib",
		"../vendor/libs/" ..outputdir .. "/ImGizmo/ImGizmo.lib",
		"../vendor/libs/" ..outputdir .. "/ImGui/ImGui.lib",
		"../vendor/libs/" ..outputdir .. "/KTX-Tools/KTX-Tools.lib",
		"../vendor/libs/" ..outputdir .. "/SPIRV-Cross/SPIRV-Cross.lib",

		"../vendor/vulkan/libs/vulkan-1.lib",
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
			"../vendor/vulkan/libs/VkLayer_utils.lib",
			"../vendor/vulkan/libs/SPIRV-Tools_d.lib",
			"../vendor/vulkan/libs/SPIRV-Tools-opt_d.lib",
			"../vendor/vulkan/libs/OGLCompiler_d.lib",
			"../vendor/vulkan/libs/OSDependent_d.lib",

			"../vendor/glslang/glslang-default-resource-limitsd.lib",
			"../vendor/glslang/glslangd.lib",
			"../vendor/glslang/GenericCodeGend.lib",
			"../vendor/glslang/MachineIndependentd.lib",
			"../vendor/glslang/SPVRemapperd.lib",
			"../vendor/glslang/SPIRVd.lib",

			"../vendor/nvidia_aftermath/lib/GFSDK_Aftermath_Lib.x64.lib",
			"../vendor/nvidia_aftermath/lib/GFSDK_Aftermath_Lib_UWP.x64.lib",

			"../vendor/ozz-animation/libs/ozz_animation_d.lib",
			"../vendor/ozz-animation/libs/ozz_animation_offline_d.lib",
			"../vendor/ozz-animation/libs/ozz_base_d.lib",
			"../vendor/ozz-animation/libs/ozz_geometry_d.lib",
			"../vendor/ozz-animation/libs/ozz_options_d.lib",
		}

		defines
		{
			"SMOLENGINE_DEBUG"
		}

	filter "configurations:Release_Vulkan"
	buildoptions "/MD"
	buildoptions "/bigobj"
	buildoptions "/Zm500"
	optimize "on"

		links 
		{ 

			"../vendor/vulkan/libs/SPIRV-Tools.lib",
			"../vendor/vulkan/libs/SPIRV-Tools-opt.lib",
			"../vendor/vulkan/libs/OGLCompiler.lib",
			"../vendor/vulkan/libs/OSDependent.lib",

			"../vendor/glslang/glslang-default-resource-limits.lib",
			"../vendor/glslang/glslang.lib",
			"../vendor/glslang/GenericCodeGen.lib",
			"../vendor/glslang/MachineIndependent.lib",
			"../vendor/glslang/SPVRemapper.lib",
			"../vendor/glslang/SPIRV.lib",

			"../vendor/ozz-animation/libs/ozz_animation_r.lib",
			"../vendor/ozz-animation/libs/ozz_animation_offline_r.lib",
			"../vendor/ozz-animation/libs/ozz_base_r.lib",
			"../vendor/ozz-animation/libs/ozz_geometry_r.lib",
			"../vendor/ozz-animation/libs/ozz_options_r.lib",
		}
