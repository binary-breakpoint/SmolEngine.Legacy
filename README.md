## Level Editor
[![SmolEngine](https://i.imgur.com/PAg7PMr.png)](https://www.youtube.com/watch?v=nW9iOi9fXbQ)

This repository contains an early version of my personal game engine - [SmolEngine](https://github.com/Floritte/SmolEngine-SDK). Feel free to use it as a reference for your own projects.

## Resources
- [Entity Component System](https://github.com/Floritte/Game-Engine-Samples/tree/main/smolengine/src/ECS)
- [C# Scripting](https://github.com/Floritte/Game-Engine-Samples/tree/main/smolengine/src/Scripting/CSharp)
- [C++ Scripting](https://github.com/Floritte/Game-Engine-Samples/tree/main/smolengine/src/Scripting/CPP)
- [Vulkan Backend](https://github.com/Floritte/Game-Engine-Samples/tree/main/smolengine.graphics/src/Backends/Vulkan)
- [2D Renderer](https://github.com/Floritte/Game-Engine-Samples/blob/main/smolengine.graphics/src/Renderer/Renderer2D.cpp)
- [3D Renderer](https://github.com/Floritte/Game-Engine-Samples/blob/main/smolengine.graphics/src/Renderer/RendererDeferred.cpp)
- [Debug Renderer](https://github.com/Floritte/Game-Engine-Samples/blob/main/smolengine.graphics/src/Renderer/RendererDebug.cpp)
- [2D Physics](https://github.com/Floritte/Game-Engine-Samples/tree/main/smolengine/src/Physics/Box2D)
- [3D Physics](https://github.com/Floritte/Game-Engine-Samples/tree/main/smolengine/src/Physics/Bullet3)
- [3D Audio](https://github.com/Floritte/Game-Engine-Samples/tree/main/smolengine/src/Audio)
- [Shaders](https://github.com/Floritte/Game-Engine-Samples/tree/main/resources/Shaders)
- [Level-Editor](https://github.com/Floritte/Game-Engine-Samples/tree/main/smolengine.editor/src)
- [Prefabs](https://github.com/Floritte/Game-Engine-Samples/blob/main/smolengine/src/ECS/Prefab.cpp)
- [Dear ImGui UI](https://github.com/Floritte/Game-Engine-Samples/blob/main/smolengine.graphics/src/Backends/Vulkan/GUI/ImGuiVulkanImpl.cpp)
- [Nuklear UI](https://github.com/Floritte/Game-Engine-Samples/blob/main/smolengine.graphics/src/Backends/Vulkan/GUI/NuklearVulkanImpl.cpp)
- [Window & Events](https://github.com/Floritte/Game-Engine-Samples/tree/main/smolengine.graphics/src/Window)
- [glTF Importer](https://github.com/Floritte/Game-Engine-Samples/blob/main/smolengine.graphics/src/Import/glTFImporter.cpp)
- [Animations](https://github.com/Floritte/Game-Engine-Samples/tree/main/smolengine.graphics/src/Animation)
- [Camera](https://github.com/Floritte/Game-Engine-Samples/tree/main/smolengine.graphics/src/Camera)
- [In-Game UI](https://github.com/Floritte/Game-Engine-Samples/tree/main/smolengine.graphics/src/GUI)
- [Materials](https://github.com/Floritte/Game-Engine-Samples/tree/main/smolengine.graphics/src/Materials)

## Building
### Windows

1. Clone the repository: ```git clone https://github.com/Floritte/SmolEngine-Legacy```
2. Install [Vulkan SDK 1.2.198.1](https://vulkan.lunarg.com/sdk/home#windows) or higher
3. Configure the solution with Premake5: run `gen-project-vs2019.bat`
4. Build the solution with Visual Studio: 
   - SmolEngine.CSharp
   - SmolEngine-Editor
   
5. Run the SmolEngine-Editor project
