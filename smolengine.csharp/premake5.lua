project "SmolEngine.CSharp"
kind "SharedLib"
language "C#"
namespace "SmolEngine"
clr "Unsafe"

targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
objdir ("../vendor/libs/bin-int/" .. outputdir .. "/%{prj.name}")

files
{
	"src/SmolEngine/**.cs",
	"src/Main.cs"
}

links 
{
	"System",
}

filter "configurations:Debug_Vulkan"
symbols "on"

filter "configurations:Release_Vulkan"
optimize "on"