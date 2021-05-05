#include "2D.h"

#include <Common/Mesh.h>
#include <Common/Input.h>
#include <Common/Text.h>
#include <ImGUI/ImGuiExtension.h>
#include <Utils/Utils.h>
#include <GraphicsContext.h>
#include <Renderer2D.h>
#include <Renderer.h>

#include <DebugRenderer.h>

#include <imgui/imgui.h>

using namespace Frostium;

int main(int argc, char** argv)
{
	GraphicsContext* context = nullptr;
	{
		WindowCreateInfo windoInfo = {};
		{
			windoInfo.bFullscreen = false;
			windoInfo.bVSync = false;
			windoInfo.Height = 480;
			windoInfo.Width = 720;
			windoInfo.Title = "Frostium 2D";
		}

		EditorCamera* camera = nullptr;
		{
			EditorCameraCreateInfo cameraCI = {};
			cameraCI.Speed = 55.0f;
			cameraCI.Type = CameraType::Ortho;
			camera = new EditorCamera(&cameraCI);
		}

		GraphicsContextInitInfo info = {};
		{
			info.Flags = Features_Renderer_2D_Flags | Features_ImGui_Flags;
			info.eMSAASamples = MSAASamples::SAMPLE_COUNT_1;
			info.ResourcesFolderPath = "../resources/";
			info.pWindowCI = &windoInfo;
			info.pDefaultCamera = camera;
		}

		context = new GraphicsContext(&info);
	}

	Text text1 = {};
	std::string str = "Frostium3D!";
	Text::CreateSDF("Assets/sdf_fonts/font_1.fnt", "Assets/sdf_fonts/font_1.png", &text1);
	text1.SetPosition({ 0, 5 });
	text1.SetSize(25.0f);
	text1.SetColor({ 0.2f, 0.7f, 1.0f, 1.0f });
	text1.SetText(str);

	Texture texture = {};
	Texture texture2 = {};
	Texture::Create("Assets/Background.png", &texture);
	Texture::Create("Assets/Bricks.png", &texture2);

	ClearInfo clearInfo = {};
	clearInfo.bClear = true;
	bool process = true;

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

		context->UpdateSceneData(); // uses default camera - updates automatically
		context->BeginFrame(deltaTime);
		{
			ImGui::Begin("Debug Window");
			{
				float lastFrameTime = deltaTime.GetTimeSeconds() * 100.0f;
				ImGui::Extensions::Text("Ms: ", std::to_string(lastFrameTime), 60.0f);
				if (ImGui::Extensions::InputRawString("Text", str, "Input", 60.0f))
					text1.SetText(str);
			}
			ImGui::End();

			Renderer2D::BeginScene(&clearInfo);
			Renderer2D::SubmitSprite(glm::vec3(10, 0, 0),  glm::vec3(10, 10, 0), { 0,0,0 }, 1, &texture2);
			Renderer2D::SubmitSprite(glm::vec3(0, 0, 0),  glm::vec3(10, 10, 0), { 0,0,0 }, 0, &texture);
			Renderer2D::SubmitSprite(glm::vec3(20, 20, 0), glm::vec3(10, 10, 0), { 0,0, 0 }, 3, &texture2);
			Renderer2D::SubmitText(&text1);
			Renderer2D::EndScene();
		}
		context->SwapBuffers();
	}
}