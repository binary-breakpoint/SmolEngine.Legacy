## Level Editor
[![SmolEngine](https://i.imgur.com/PAg7PMr.png)](https://www.youtube.com/watch?v=nW9iOi9fXbQ)

This repository contains an early version of my personal game engine - [SmolEngine](https://github.com/Floritte/SmolEngine-SDK). Feel free to use it as a reference for your own projects.

## Notable Features
  - C#/C++ Scripting API
  - Entity Component System
  - Jobs System
  - Audio
  - Powerful Level Editor
  - Prefabs
  - Physics: 2D ([Box2D](https://github.com/erincatto/box2d)), 3D ([Bullet3](https://github.com/bulletphysics/bullet3))
  - Integrated Profilers: [Optick](https://github.com/bombomby/optick), [Nsight Graphics](https://developer.nvidia.com/nsight-graphics/)
  
## Notable Features: Rendering
  - Own abstraction layer on top of OpenGL/Vulkan API
  - Physically Based Rendering (Metalness-Roughness workflow)
  - Deferred Rendering
  - Shadow Mapping
  - Material System (custom shaders)
  - Skeleton Animations
  - In-Game UI (text, buttons, input fields, etc)
  - ImGUI Integration
  - Compute Pipelines
  - Bloom

## Building
### Windows

1. Clone the repository: ```git clone https://github.com/Floritte/SmolEngine-Legacy```
2. Install [Vulkan SDK 1.2.198.1](https://vulkan.lunarg.com/sdk/home#windows) or higher
3. Configure the solution with Premake5: run `gen-project-vs2019.bat`
4. Build the solution with Visual Studio: 
   - SmolEngine.CSharp
   - SmolEngine-Editor
   
5. Run the SmolEngine-Editor project
