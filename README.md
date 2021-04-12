Frostium3D
=====
![SmolEngine](https://i.imgur.com/W81qlzQ.png)
### Current State
  - Multiple rendering API backends: Vulkan - 100%, OpenGL - 70%
  - Physically Based Rendering (PBR)
  - Deferred Shading
  - Shadow Mapping
  - MSAA / FXAA
  - SSAO
  - 2D and 3D renderers
  - ImGUI Integration
  - Text Rendering (SDF)
### Usage
The first step is to initialize graphics context class:
```cpp
#include <Common/Mesh.h>
#include <GraphicsContext.h>
#include <Renderer.h>

#include <imgui/imgui.h>

using namespace Frostium;

int main(int argc, char** argv)
{
	GraphicsContext* context = nullptr;
	WindowCreateInfo windowCI = {};
	{
		windowCI.bFullscreen = false;
		windowCI.bVSync = false;
		windowCI.Height = 480;
		windowCI.Width = 720;
		windowCI.Title = "Frostium Example";
	}

	EditorCameraCreateInfo cameraCI = {}; // default camera
	GraphicsContextInitInfo info = {};
	{
		info.Flags = Features_Renderer_3D_Flags | Features_ImGui_Flags;
		info.eMSAASamples = MSAASamples::SAMPLE_COUNT_MAX_SUPPORTED;
		info.ResourcesFolderPath = "../resources/";
		info.pWindowCI = &windoInfo;
		info.pEditorCameraCI = &cameraCI;
	}

	context = new GraphicsContext(&info);
}
```
Once graphics context is initialized, set event callback:
```cpp
	bool process = true;
	context->SetEventCallback([&](Event& e)
	{
		if(e.IsType(EventType::WINDOW_CLOSE))
			process = false;
	});
```
And finally load resources and run main update loop:
```cpp

	ClearInfo clearInfo = {};
	Mesh cube = {};
	Mesh::Create("Assets/cube.glb", &cube);
  
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
				std::string str = "DeltaTime: " + std::to_string(lastFrameTime);
				ImGui::Text(str.c_str());
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
```
More samples can be found [here.](https://github.com/YellowDummy/Frostium3D/tree/main/samples)

## Third-Party Libraries
- [vulkan](https://www.lunarg.com/vulkan-sdk/)
- [glad](https://glad.dav1d.de/)
- [glfw](https://github.com/glfw/glfw)
- [spirv-cross](https://github.com/KhronosGroup/SPIRV-Cross)
- [stb_image](https://github.com/nothings/stb)
- [ktx](https://github.com/KhronosGroup/KTX-Software)
- [glm](https://github.com/g-truc/glm)
- [gli](https://github.com/g-truc/gli)
- [assimp](https://github.com/assimp/assimp)
- [imgui](https://github.com/ocornut/imgui)

## Limitations
- Developed by one person for learning purposes
- At a very early stage
- No documentation yet

## Building
### Windows
1. Install Vulkan SDK (1.2 or higher)
2. Run gen-project-vs2019.bat or cmd ```premake5 vs2019```
3. Extract libs.7z in vendor\vulkan
4. Extract libs.7z in vendor\assimp
5. Compile
