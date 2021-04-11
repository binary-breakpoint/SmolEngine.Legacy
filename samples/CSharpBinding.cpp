#include "CSharpBinding.h"

#include <GraphicsContext.h>
#include <Renderer.h>

#include <mono/metadata/assembly.h>
#include <mono/jit/jit.h>

using namespace Frostium;

GraphicsContext* context = nullptr;

void PrintMethod(MonoString* string)
{
	char* cppString = mono_string_to_utf8(string);
	NATIVE_INFO(cppString);
	mono_free(cppString);
}

void SetWindow(MonoString* title, int32_t width, int32_t height)
{
	std::string str(mono_string_to_utf8(title));
	context->GetWindow()->SetTitle(str);
	context->GetWindow()->SetWidth(width);
	context->GetWindow()->SetHeight(height);
}

void InitMono()
{
	//Indicate Mono where you installed the lib and etc folders
	mono_set_dirs("../vendor/mono/lib", "../vendor/mono/etc");
	//Create the main CSharp domain
	MonoDomain* domain = mono_jit_init("CSharp_Domain");
	//Load the binary file as an Assembly
	MonoAssembly* csharpAssembly = mono_domain_assembly_open(domain, "../vendor/mono/CSharp/Debug/CSharp.exe");
	if (!csharpAssembly)
	{
		//Error detected
		return;
	}

	//SetUp Internal Calls called from CSharp

	//Namespace.Class::Method + a Function pointer with the actual definition
	mono_add_internal_call("Frostium.FrostiumEngine::PrintMethod", &PrintMethod);
	//Namespace.Class::Method + a Function pointer with the actual definition
	mono_add_internal_call("Frostium.FrostiumEngine::SetWindow", &SetWindow);

	int argc = 1;
	char* argv[1] = { (char*)"CSharp" };

	//Call the main method in this code
	mono_jit_exec(domain, csharpAssembly, argc, argv);
}

int main(int argc, char** argv)
{
	WindowCreateInfo windoInfo = {};
	{
		windoInfo.bFullscreen = false;
		windoInfo.bVSync = false;
		windoInfo.Height = 480;
		windoInfo.Width = 720;
		windoInfo.Title = "Frostium Example";
	}

	EditorCameraCreateInfo cameraCI = {};
	GraphicsContextInitInfo info = {};
	{
		info.Flags = Features_Renderer_3D_Flags;
		info.ResourcesFolderPath = "../resources/";
		info.pWindowCI = &windoInfo;
		info.pEditorCameraCI = &cameraCI;
	}

	bool process = true;
	ClearInfo clearInfo = {};
	context = new GraphicsContext(&info);
	context->SetEventCallback([&](Event& e)
		{
			if (e.IsType(EventType::WINDOW_CLOSE))
				process = false;
		});

	InitMono();

	while (process)
	{
		context->ProcessEvents();
		DeltaTime deltaTime = context->CalculateDeltaTime();

		if (context->IsWindowMinimized())
			continue;

		context->BeginFrame(deltaTime);
		{
			Renderer::BeginScene(&clearInfo);
			Renderer::EndScene();
		}
		context->SwapBuffers();
	}
}