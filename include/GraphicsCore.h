#pragma once
#include "Core/Debug/DebugLog.h"
#include "Core/Multithreading/JobsSystem.h"

#include "Graphics/GraphicsContext.h"
#include "Graphics/Primitives/Framebuffer.h"
#include "Graphics/Primitives/Mesh.h"
#include "Graphics/Primitives/Shader.h"
#include "Graphics/Primitives/Texture.h"
#include "Graphics/Primitives/VertexArray.h"
#include "Graphics/Primitives/VertexBuffer.h"
#include "Graphics/Primitives/IndexBuffer.h"
#include "Graphics/Primitives/GraphicsPipeline.h"
#include "Graphics/Primitives/ComputePipeline.h"
#include "Graphics/Primitives/RaytracingPipeline.h"
#include "Graphics/Primitives/EnvironmentMap.h"

#include "Graphics/Pools/MaterialPool.h"
#include "Graphics/Pools/MeshPool.h"
#include "Graphics/Pools/TexturePool.h"
#include "Graphics/Pools/PipelinePool.h"

#include "Graphics/GUI/UICanvas.h"
#include "Graphics/GUI/UIButton.h"
#include "Graphics/GUI/UIText.h"
#include "Graphics/GUI/UISlider.h"
#include "Graphics/GUI/UITextField.h"
#include "Graphics/GUI/UICheckbox.h"

#include "Graphics/Common/Flags.h"
#include "Graphics/Window/Events.h"
#include "Graphics/Window/Input.h"

#include "Graphics/Camera/EditorCamera.h"
#include "Graphics/Tools/Utils.h"
#include "Graphics/Import/glTFImporter.h"
#include "Graphics/Import/OzzImported.h"

#include "Graphics/Renderer/RendererDeferred.h"
#include "Graphics/Renderer/RendererDebug.h"
#include "Graphics/Renderer/Renderer2D.h"

#include "Graphics/Materials/Material3D.h"
#include "Graphics/Materials/Material2D.h"
#include "Graphics/Materials/PBRFactory.h"
#include "Graphics/Animation/AnimationController.h"

#include <imgui/imgui.h>