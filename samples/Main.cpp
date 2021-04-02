#include "Main.h"

#include <string>

#include "GraphicsContext.h"
#include "Renderer.h"
#include "Common/EventHandler.h"

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
		info.WindowInfo = &windoInfo;
	}

	GraphicsContext::Init(&info);
	GraphicsContext::SetEventCallback([&](Event&)
	{
		
	});

	BeginSceneInfo bInfo = {};
	bInfo.farClip = 1000.0f;
	bInfo.nearClip = 0.01f;
	bInfo.pos = { 0, 0, -6 };
	bInfo.proj = glm::mat4(1.0f);
	bInfo.view = glm::mat4(1.0f);

	while (true)
	{
		GraphicsContext::BeginFrame();

		Renderer::BeginScene(bInfo);
		Renderer::EndScene();

		GraphicsContext::SwapBuffers();
	}
}