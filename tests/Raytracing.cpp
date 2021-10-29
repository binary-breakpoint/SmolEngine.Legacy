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

	BufferLayout mainLayout =
	{
		{ DataTypes::Float3, "aPos" },
		{ DataTypes::Float3, "aNormal" },
		{ DataTypes::Float3, "aTangent" },
		{ DataTypes::Float2, "aUV" },
		{ DataTypes::Int4,   "aBoneIDs"},
		{ DataTypes::Float4, "aWeight"}
	};

	RaytracingPipelineCreateInfo rtCreateInfo{};
	rtCreateInfo.ShaderCI.Stages[ShaderType::RayCloseHit] = info.ResourcesFolder + "Shaders/Raytracing/basic.rchit";
	rtCreateInfo.ShaderCI.Stages[ShaderType::RayGen] = info.ResourcesFolder + "Shaders/Raytracing/basic.rgen";
	rtCreateInfo.ShaderCI.Stages[ShaderType::RayMiss] = info.ResourcesFolder + "Shaders/Raytracing/basic.rmiss";
	rtCreateInfo.VertexInput = VertexInputInfo(sizeof(PBRVertex), mainLayout);

	Ref<RaytracingPipeline> rtPipeline = RaytracingPipeline::Create();
	rtPipeline->Build(&rtCreateInfo);


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
