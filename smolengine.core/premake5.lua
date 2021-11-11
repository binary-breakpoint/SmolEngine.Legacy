project "SmolEngine.Core"
kind "StaticLib"
language "C++"
cppdialect "C++17"
staticruntime "on"

targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
objdir ("../vendor/libs/bin-int/" .. outputdir .. "/%{prj.name}")

files
{
	"include/**.h",
	"src/**.cpp",
	
	"../smolengine.external/glm/glm/**.hpp",
	"../smolengine.external/glm/glm/**.inl",

	"../vendor/stb_image/**.cpp",
}

includedirs
{
	"../smolengine.external/spdlog/include",
	"../smolengine.external",

	"include/",
}

filter "configurations:Debug_Vulkan"
buildoptions "/MDd"
buildoptions "/bigobj"
buildoptions "/Zm500"
symbols "on"

filter "configurations:Release_Vulkan"
buildoptions "/MD"
buildoptions "/bigobj"
buildoptions "/Zm500"
optimize "on"