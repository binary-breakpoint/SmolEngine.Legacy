#include "Main.h"

#include <Common/Mesh.h>
#include <Common/Input.h>
#include <GraphicsContext.h>
#include <Renderer.h>

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

	EditorCameraCreateInfo cameraCI = {};
	GraphicsContextInitInfo info = {};
	{
		info.bMSAA = true;
		info.bTargetsSwapchain = true;
		info.ResourcesFolderPath = "../resources/";
		info.pWindowCI = &windoInfo;
		info.pEditorCameraCI = &cameraCI;
	}

	GraphicsContext* context = new GraphicsContext(&info);
	ClearInfo clearInfo = {};
	Mesh cube = {};
	bool process = true;

	Mesh::Create("Assets/cube.glb", &cube);

	context->SetEventCallback([&](Event& e) 
	{
		if(e.IsType(EventType::WINDOW_CLOSE))
			process = false;
	});

	while (process)
	{
		context->ProcessEvents();
		DeltaTime deltaTime = context->CalculateDeltaTime();

		if (context->IsWindowMinimized())
			continue;

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
			{
				Renderer::SubmitMesh({ 0, 0, -10 }, { 0, 0, 0 }, { 1, 8, 1 }, &cube);
				Renderer::SubmitMesh(  { 0, 0, 0 }, { 0, 0, 0 }, { 1, 6, 1 }, &cube);
				Renderer::SubmitMesh( { 0, 0, 10 }, { 0, 0, 0 }, { 1, 4, 1 }, &cube);
				Renderer::SubmitMesh( { 0, 0, 20 }, { 0, 0, 0 }, { 1, 2, 1 }, &cube);
			}
			Renderer::EndScene();
		}
		context->SwapBuffers();
	}
}