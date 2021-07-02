Screenshots
=====
![Sponza](https://i.imgur.com/2hoI5Wt.png)
![PBR](https://i.imgur.com/Pzsbolf.png)
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
  - ImGUI Integration
  - Text Rendering (SDF)
  - glTF 2.0
  - Skeleton Animations

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
#include <Common/Mesh.h>
#include <GraphicsContext.h>
#include <DeferredRenderer.h>

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

	Camera* camera = nullptr; // default camera
	{
		EditorCameraCreateInfo cameraCI = {};
		camera = new EditorCamera(&cameraCI);
	}

	GraphicsContextInitInfo info = {};
	{
		info.Flags = Features_Renderer_3D_Flags | Features_ImGui_Flags;
		info.eShadowMapSize = ShadowMapSize::SIZE_8;
		info.ResourcesFolderPath = "../resources/";
		info.pWindowCI = &windoInfo;
		info.pDefaultCamera = camera;
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
	Mesh::Create("Assets/cube.gltf", &cube);
  
	while (process)
	{
		context->ProcessEvents();

		if (context->IsWindowMinimized())
			continue;
      
		/* 
		   @Simulate physics, process scripts, etc
		*/
		
		// Far clip, near clip, camera position, projection and view matrices
		BeginSceneInfo info = {};
		// Updates struct using camera class
		info.Update(camera);
		
		// Calculates default frustum, updates buffers
		context->UpdateSceneInfo(&info);
		
		DeltaTime deltaTime = context->CalculateDeltaTime();
		context->BeginFrame(deltaTime);
		{
			uint32_t objects = 0;
			DeferredRenderer::BeginScene(&clearInfo);
			{
				for (const auto& c : chunks)
					DeferredRenderer::SubmitMesh(c.Pos, c.Rot, c.Scale, &cube, c.MaterialID);
					
				objects = DeferredRenderer::GetNumObjects();
			}
			DeferredRenderer::EndScene();
			
			ImGui::Begin("Debug Window");
			{
				float lastFrameTime = deltaTime.GetTimeSeconds();
				std::string str = "DeltaTime: " + std::to_string(lastFrameTime);
				std::string str2 = "Objects: " + std::to_string(objects);
				ImGui::Text(str.c_str());
				ImGui::Text(str2.c_str());
			}
			ImGui::End();
		}
		context->SwapBuffers();
	}
```
![result](https://i.imgur.com/EOYtgZ2.png)

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
