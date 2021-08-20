Screenshots
=====
![Sponza](https://i.imgur.com/2hoI5Wt.png)
## Core Features
  - Multiple rendering API backends (Vulkan - 100%, OpenGL - 80%)
  - Own abstraction layer on top of OpenGL/Vulkan API
  - Physically Based Rendering (Metalness-Roughness workflow)
  - Deferred Shading Rendering Path
  - Dynamic/Static Environment Maps
  - Full HDR Lighting
  - Anti-aliasing: MSAA and FXAA
  - Post-Processing: Bloom, Blur
  - IBL, Point/Spot/Directional Lighting
  - Basic Shadow Mapping
  - 2D/3D/Debug renderers
  - Skeleton Animations
  - Text Rendering (SDF)
  - ImGUI Integration
  - glTF 2.0
  - Multithreading

## Features to come

- Water and Terrain Rendering
- Particle System
- Global Illumination
- Real-Time Ray Tracing
- Android/IOS/Linux Support
- Dynamic Day/Night Cycle
- Weather System
- Unit Tests
- Documentation

## Usage
The first step is to initialize graphics context class:
```cpp
#include <FrostiumCore.h>

using namespace Frostium;

int main(int argc, char** argv)
{
	WindowCreateInfo windoInfo = {};
	{
		windoInfo.bFullscreen = false;
		windoInfo.bVSync = false;
		windoInfo.Height = 480;
		windoInfo.Width = 720;
		windoInfo.Title = "Frostium PBR";
	}

	Camera* camera = nullptr;
	{
		EditorCameraCreateInfo cameraCI = {};
		camera = new EditorCamera(&cameraCI);
	}

	GraphicsContextInitInfo info = {};
	{
		info.eMSAASamples = MSAASamples::SAMPLE_COUNT_1;
		info.ResourcesFolderPath = "../resources/";
		info.pWindowCI = &windoInfo;
		info.pDefaultCamera = camera;
	}

	bool process = true;
	GraphicsContext* context = new GraphicsContext(&info);
	context->SetDebugLogCallback([&](const std::string&& msg, LogLevel level) { std::cout << msg << "\n"; });
	context->SetEventCallback([&](Event& e) { if (e.IsType(EventType::WINDOW_CLOSE)) { process = false; } });

	RendererStorage* storage = new RendererStorage();
	storage->Initilize();
	context->PushStorage(storage);

	RendererDrawList* drawList = new RendererDrawList();
	SceneViewProjection viewProj = SceneViewProjection(camera);
	ClearInfo clearInfo;
}
```
The next step is to create a draw list. Since this example uses static objects, there is no need to update them every frame:
```cpp
	Mesh cube = {};
	Mesh::Create("Assets/cube.gltf", &cube);
	
	drawList->GetFrustum().SetRadius(1000.0f);
	drawList->BeginSubmit(viewProj);
	{
		for (const auto& c : chunks)
		{
		    drawList->SubmitMesh(c.Pos, c.Rot, c.Scale, &cube, c.MaterialID);
		}
	}
	drawList->EndSubmit();
```
And finally run main update loop:
```cpp
	DirectionalLight dirLight = {};
	dirLight.IsActive = true;
	dirLight.Direction = glm::vec4(105.0f, 53.0f, 102.0f, 0);
	drawList->SubmitDirLight(&dirLight);

	while (process)
	{
		context->ProcessEvents();
		float deltaTime = context->CalculateDeltaTime();

		if (context->IsWindowMinimized())
			continue;

		/* 
		   Calculates physics, update scripts, etc.
		*/

		viewProj.Update(camera);

		context->BeginFrame(deltaTime);
		{
			ImGui::Begin("PBR Sample");
			{
			    ImGui::Text("Some Text");
			}
			ImGui::End();

			RendererDeferred::DrawFrame(&clearInfo, storage, drawList);
		}
		context->SwapBuffers();
	}
```
![result](https://i.imgur.com/jz2yysp.png)

More samples can be found [here.](https://github.com/YellowDummy/Frostium3D/tree/main/samples)

## Third-Party Libraries
- [vulkan](https://www.lunarg.com/vulkan-sdk/)
- [glad](https://glad.dav1d.de/)
- [glfw](https://github.com/glfw/glfw)
- [taskflow](https://github.com/taskflow/taskflow)
- [tinygltf](https://github.com/syoyo/tinygltf)
- [spirv-cross](https://github.com/KhronosGroup/SPIRV-Cross)
- [stb_image](https://github.com/nothings/stb)
- [spdlog](https://github.com/gabime/spdlog)
- [cereal](https://github.com/USCiLab/cereal)
- [ktx](https://github.com/KhronosGroup/KTX-Software)
- [glm](https://github.com/g-truc/glm)
- [gli](https://github.com/g-truc/gli)
- [imgui](https://github.com/ocornut/imgui)
- [implot](https://github.com/epezent/implot)
- [imgizmo](https://github.com/CedricGuillemet/ImGuizmo)

## Limitations
- Developed by one person for learning purposes
- At a very early stage
- No documentation yet

## Building
### Windows
1. Install [Vulkan SDK](https://www.lunarg.com/vulkan-sdk/)
2. Run gen-project-vs2019.bat or cmd ```premake5 vs2019```
3. Extract libs.7z from vendor\vulkan
4. Compile