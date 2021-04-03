#include "Main.h"

#include <GraphicsContext.h>
#include <Renderer.h>
#include <string>

using namespace Frostium;

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

	GraphicsContextInitInfo info = {};
	{
		info.bMSAA = true;
		info.bTargetsSwapchain = true;
		info.WindowCI = &windoInfo;
		info.ResourcesFolderPath = "../resources/";
	}

	GraphicsContext* context = new GraphicsContext(&info);
	ClearInfo clearInfo = {};

	context->SetEventCallback([&](Event&) {});

	while (true)
	{
		context->ProcessEvents();
		DeltaTime deltaTime = context->CalculateDeltaTime();

		/* 
		   @Calculate physics, process script, etc
		*/

		context->BeginFrame(deltaTime);
		{
			ImGui::Begin("Debug Window");
			{
				float lastFrameTime = deltaTime.GetTimeSeconds();
				ImGui::InputFloat("DeltaTime", &lastFrameTime, 0.0f, 0.0f, "%.3f", ImGuiInputTextFlags_ReadOnly);
			}
			ImGui::End();

			Renderer::BeginScene(&clearInfo);
			Renderer::EndScene();
		}
		context->SwapBuffers();
	}
}