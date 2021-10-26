#include "Raytracing.h"

#include <GraphicsCore.h>

using namespace SmolEngine;
int main(int argc, char** argv)
{
	EditorCameraCreateInfo cameraCI = {};
	Camera* camera = new EditorCamera(&cameraCI);
	DebugLog* log = new DebugLog([&](const std::string&& msg, LogLevel level) { std::cout << msg << "\n"; });

	WindowCreateInfo windoInfo = {};
	{
		windoInfo.bFullscreen = false;
		windoInfo.bVSync = false;
		windoInfo.Height = 480;
		windoInfo.Width = 720;
		windoInfo.Title = "Raytracing";
	}

	GraphicsContextCreateInfo info = {};
	info.ResourcesFolder = "../resources/";
	info.pWindowCI = &windoInfo;
	info.eFeaturesFlags = {};

	auto context = GraphicsContext::Create(&info);
	context->SetEventCallback([&](Event& e) 
     { 
         camera->OnEvent(e);
     });

	//ShaderCreateInfo shaderCI{};
	//
	//shaderCI.Stages[ShaderType::RayHit] = info.ResourcesFolder + "Shaders/Raytracing/basic.rchit";
	//shaderCI.Stages[ShaderType::RayGen] = info.ResourcesFolder + "Shaders/Raytracing/basic.rgen";
	//shaderCI.Stages[ShaderType::RayMiss] = info.ResourcesFolder + "Shaders/Raytracing/basic.rmiss";
	//
	//Ref<Shader> shader = Shader::Create();
	//shader->Build(&shaderCI);


	while (context->IsOpen())
	{
		context->ProcessEvents();
		float deltaTime = context->CalculateDeltaTime();

		if (context->IsWindowMinimized())
			continue;

		context->BeginFrame(deltaTime);

		context->SwapBuffers();
	}
}
