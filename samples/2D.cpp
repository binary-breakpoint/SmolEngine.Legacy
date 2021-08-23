#include "2D.h"

#include <FrostiumCore.h>

#ifdef FROSTIUM_SMOLENGINE_IMPL
using namespace SmolEngine;
#else
using namespace Frostium;
#endif


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
	GraphicsContext*  context = new GraphicsContext(&info);
	context->SetDebugLogCallback([&](const std::string&& msg, LogLevel level) { std::cout << msg << "\n"; });
	context->SetEventCallback([&](Event& e) { if (e.IsType(EventType::WINDOW_CLOSE)) { process = false; } camera->OnEvent(e); });

	Renderer2DStorage* storage = new Renderer2DStorage();
	storage->Initilize();
	context->PushStorage(storage);

	RendererDrawList2D* drawList = new RendererDrawList2D();

	Text text1 = {};
	std::string str = "Frostium3D!";
	Text::CreateSDF("Assets/sdf_fonts/font_1.fnt", "Assets/sdf_fonts/font_1.png", &text1);
	text1.SetPosition({ 0, 5 });
	text1.SetSize(25.0f);
	text1.SetColor({ 0.2f, 0.7f, 1.0f, 1.0f });
	text1.SetText(str);

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
		drawList->SubmitText(&text1);
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