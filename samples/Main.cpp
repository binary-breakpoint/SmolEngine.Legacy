#include "Main.h"

#include <Common/Mesh.h>
#include <Common/Input.h>
#include <Utils/Utils.h>
#include <GraphicsContext.h>
#include <Renderer.h>

using namespace Frostium;

struct Chunk
{
	glm::vec3 Pos = glm::vec3(1.0f);
	glm::vec3 Rot = glm::vec3(0.0f);
	glm::vec3 Scale = glm::vec3(1.0f);
};

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
		info.bImGUI = true;
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

	std::vector<Chunk> chunks;
	Chunk chunk = {};
	for (uint32_t x = 0; x < 50; x+=2)
	{
		for (uint32_t z = 0; z < 50; z+=2)
		{
			float scale_y = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 6));

			chunk.Pos = { x, 0, z };
			chunk.Rot = { 0, 0, 0 };
			chunk.Scale = { 0.5, scale_y, 0.5 };

			chunks.emplace_back(chunk);
		}
	}

	while (process)
	{
		context->ProcessEvents();
		DeltaTime deltaTime = context->CalculateDeltaTime();

		if (context->IsWindowMinimized())
			continue;

		if (Input::IsMouseButtonPressed(MouseCode::ButtonRight))
		{
			float rayDistance = 50.0f;
			float y = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 6));

			glm::vec3 startPos = context->GetEditorCamera()->GetPosition();
			glm::mat4 viewProj = context->GetEditorCamera()->GetViewProjection();

			{
				glm::vec3 pos = Utils::CastRay(startPos, rayDistance, viewProj);

				chunk.Pos = pos;
				chunk.Rot = { 0, 0, 0 };
				chunk.Scale = { 0.5, y, 0.5 };
				chunks.emplace_back(chunk);
			}
		}

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
				for (const auto& c : chunks)
					Renderer::SubmitMesh(c.Pos, c.Rot, c.Scale, &cube);
			}
			Renderer::EndScene();
		}
		context->SwapBuffers();
	}
}