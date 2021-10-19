#include "2D.h"

#include <GraphicsCore.h>

using namespace SmolEngine;

int main(int argc, char** argv)
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
		info.eMSAASamples = MSAASamples::SAMPLE_COUNT_1;
		info.ResourcesFolderPath = "../resources/";
		info.pWindowCI = &windoInfo;
	}

	bool process = true;
	DebugLog* log = new DebugLog([&](const std::string&& msg, LogLevel level) { std::cout << msg << "\n"; });
	GraphicsContext*  context = new GraphicsContext(&info);
	context->SetEventCallback([&](Event& e) { if (e.IsType(EventType::WINDOW_CLOSE)) { process = false; } camera->OnEvent(e); });

	Renderer2DStorage* storage = new Renderer2DStorage();
	storage->Initilize();
	context->PushStorage(storage);

	RendererDrawList2D* drawList = new RendererDrawList2D();;

	Texture texture = {};
	Texture texture2 = {};
	TextureCreateInfo textureCI = {};

	textureCI.FilePath = "Assets/Background.png";
	Texture::Create(&textureCI, &texture);

	textureCI.FilePath = "Assets/Bricks.png";
	Texture::Create(&textureCI, &texture2);

	ClearInfo clearInfo = {};
	clearInfo.bClear = true;

	SceneViewProjection viewProj;

	while (process)
	{
		context->ProcessEvents();
		float deltaTime = context->CalculateDeltaTime();

		if (context->IsWindowMinimized())
			continue;

		/* 
		    Calculates physics, update scripts, etc.
		*/

		camera->OnUpdate(deltaTime);
		viewProj.Update(camera);

		drawList->BeginSubmit(&viewProj);
		drawList->SubmitSprite(glm::vec3(10, 0, 0), glm::vec3(10, 10, 0), { 0,0,0 }, 1, &texture2, false);
		drawList->SubmitSprite(glm::vec3(0, 0, 0), glm::vec3(10, 10, 0), { 0,0,0 }, 0, &texture, false);
		drawList->SubmitSprite(glm::vec3(20, 20, 0), glm::vec3(10, 10, 0), { 0,0, 0 }, 3, &texture2, false);
		drawList->EndSubmit();

		context->BeginFrame(deltaTime);
		{
			ImGui::Begin("2D Sample");
			{

			}
			ImGui::End();

			Renderer2D::DrawFrame(&clearInfo, storage, drawList);
		}
		context->SwapBuffers();
	}
}