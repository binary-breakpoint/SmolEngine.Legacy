#pragma once

#include "Common/Memory.h"
#include "Common/Flags.h"
#include "Window/Window.h"
#include "Window/Events.h"
#include "Camera/EditorCamera.h"
#include "Camera/Camera.h"
#include "Pools/MeshPool.h"
#include "Pools/TexturePool.h"
#include "Pools/MaterialPool.h"
#include "Primitives/Framebuffer.h"

#include "Multithreading/JobsSystemInstance.h"

#include <functional>

namespace SmolEngine
{
	struct WindowCreateInfo;
	struct RendererStorageBase;

	struct GraphicsContextCreateInfo
	{
		bool              bTargetsSwapchain = true;
		bool              bAutoResize = true;
		bool              bVsync = true;
		std::string       ResourcesFolder = "../resources/";
		FeaturesFlags     eFeaturesFlags = FeaturesFlags::Imgui | FeaturesFlags::RendererDebug;
		MSAASamples       eMSAASamples = MSAASamples::SAMPLE_COUNT_1;
		WindowCreateInfo* pWindowCI = nullptr;
	};

	struct SceneViewProjection
	{
		SceneViewProjection() = default;
		SceneViewProjection(Camera* cam)
		{
			Update(cam);
		}

		void Update(Camera* cam)
		{
			View = cam->GetViewMatrix();
			Projection = cam->GetProjection();
			CamPos = glm::vec4(cam->GetPosition(), 1.0f);
			NearClip = cam->GetNearClip();
			FarClip = cam->GetFarClip();
			SkyBoxMatrix = glm::mat4(glm::mat3(View));
		}

		glm::mat4 Projection = glm::mat4(1.0f);
		glm::mat4 View = glm::mat4(1.0f);
		glm::vec4 CamPos = glm::vec4(1.0f);
		float     NearClip = 0.0f;
		float     FarClip = 0.0f;
		glm::vec2 Pad1;
		glm::mat4 SkyBoxMatrix = glm::mat4(1.0f);

		friend struct RendererStorage;
		friend struct RendererDrawList;
	};

	class GraphicsContext
	{
	public:
		GraphicsContext();
		virtual ~GraphicsContext();

		static Ref<GraphicsContext>       Create(GraphicsContextCreateInfo* info);
								      
		void                              ProcessEvents();
		void                              BeginFrame(float time);
		void                              SwapBuffers();
		void                              Shutdown();      		          
		static GraphicsContext*           GetSingleton();
		Ref<Framebuffer>                  GetMainFramebuffer() const;
		Window*                           GetWindow() const;
		float                             GetGltfTime() const;
		float                             GetDeltaTime() const;
		const std::string&                GetResourcesPath() const;							      	          
		void                              SetEventCallback(std::function<void(Event&)> callback);
		void                              SetFramebufferSize(uint32_t width, uint32_t height);
		float                             CalculateDeltaTime();
		bool                              IsWindowMinimized() const;
		void                              Resize(uint32_t* width, uint32_t* height);
									      
	private:		
		virtual void                      SwapBuffersEX() = 0;
		virtual void                      BeginFrameEX(float time) = 0;
		virtual void                      ShutdownEX() = 0;
		virtual void                      ResizeEX(uint32_t* width, uint32_t* height) = 0;
		virtual void                      CreateAPIContextEX() = 0;
		virtual void                      OnEventEX(Event& event) = 0;
		virtual void                      OnContexReadyEX() = 0;
		void                              Initialize(GraphicsContextCreateInfo* info);
		void                              OnEvent(Event& event);

	private:	
		bool                              m_bWindowMinimized = false;
		float                             m_LastFrameTime = 1.0f;
		float                             m_DeltaTime = 0.0f;
		static GraphicsContext*           s_Instance;
		Ref<Framebuffer>                  m_Framebuffer = nullptr;
		Ref<MaterialPool>                 m_MaterialPool = nullptr;
		Ref<MeshPool>                     m_MeshPool = nullptr;
		Ref<TexturePool>                  m_TexturePool = nullptr;
		Ref<JobsSystemInstance>           m_JobsSystem = nullptr;
		Ref<Window>                       m_Window = nullptr;	
		std::string                       m_Root = "";
		EventSender                       m_EventHandler = {};
		GraphicsContextCreateInfo         m_CreateInfo = {};
		std::function<void(Event&)>       m_EventCallback;
		std::vector<RendererStorageBase*> m_StorageList;

		friend struct RendererStorage;
		friend struct Renderer2DStorage;
		friend struct RendererStorageBase;
		friend class GraphicsPipeline;
		friend class DebugRenderer;
		friend class ImGuiContext;
		friend class VulkanPBR;
		friend class VulkanContext;
		friend class VulkanDescriptor;
		friend class Window;
		friend class EnvironmentMap;
	};
}
