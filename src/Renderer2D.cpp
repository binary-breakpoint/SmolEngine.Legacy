#include "stdafx.h"
#include "Renderer2D.h"
#include "GraphicsPipeline.h"

#include "Common/Mesh.h"
#include "Utils/Utils.h"

namespace Frostium
{
	struct Instance
	{
		Texture* Texture;

		glm::vec3 Position;
		glm::vec3 Rotation;
		glm::vec3 Scale;
		glm::vec4 Color;
	};

	struct ShaderInstance
	{
		glm::mat4  Model;
		glm::vec4  Color;
		glm::ivec4 Params; // x - texture index
	};

	struct Renderer2DStorage
	{
		Renderer2DStorage()
		{
			Tetxures.resize(Renderer2D::MaxTextureSlot);
		}

		~Renderer2DStorage()
		{

		}

		// Limits
		static const uint32_t     Light2DBufferMaxSize = 100;
		static const uint32_t     MaxQuads = 15000;
		static const uint32_t     MaxLayers = 12;

		// Count tracking
		uint32_t                  InstIndex = 0;
		uint32_t                  TexIndex = 1; // 0 - white dummy texture

		// Pipelines
		GraphicsPipeline          DeferredPipeline = {};
		GraphicsPipeline          CombinationPipeline = {};
		GraphicsPipeline          TextPipeline = {};

		// Meshes
		Mesh                      PlaneMesh = {};

		// Framebuffers
		Framebuffer               DeferredFB = {};
		Framebuffer*              MainFB = nullptr;

		// UBO's and SSBO's
		SceneData                 m_SceneData = {};
		ShaderInstance            ShaderInstances[MaxQuads];

		// Buffers
		Instance                  Instances[MaxQuads];
		std::vector<Texture*>     Tetxures;

		// Bindings
		const uint32_t     StorageBufferBP = 1;
		const uint32_t     SamplersBP = 2;

		// Sizes
		const size_t       ShaderInstanceSize = sizeof(ShaderInstance);

	};

	static Renderer2DStorage* s_Data;

	void Renderer2D::Init()
	{
		Stats = new Renderer2DStats();
		s_Data = new Renderer2DStorage();

		CreateFramebuffers();
		CreatePipelines();
	}

	void Renderer2D::Shutdown()
	{
		delete s_Data, Stats;
	}

	void Renderer2D::BeginScene(const ClearInfo* clearInfo, const BeginSceneInfo* info)
	{
		if (GraphicsContext::GetSingleton()->m_UseEditorCamera)
		{
			EditorCamera* camera = GraphicsContext::GetSingleton()->GetEditorCamera();
			s_Data->m_SceneData.View = camera->GetViewMatrix();
			s_Data->m_SceneData.Projection = camera->GetProjection();
		}

		if (info)
		{
			s_Data->m_SceneData.View = info->view;
			s_Data->m_SceneData.Projection = info->proj;
		}

		s_Data->CombinationPipeline.BeginCommandBuffer(true);
		s_Data->DeferredPipeline.BeginCommandBuffer(true);
		//s_Data->TextPipeline.BeginCommandBuffer(true);

		if (clearInfo->bClear)
		{
			s_Data->DeferredPipeline.BeginRenderPass();
			s_Data->DeferredPipeline.ClearColors(clearInfo->color);
			s_Data->DeferredPipeline.EndRenderPass();

			s_Data->CombinationPipeline.BeginRenderPass();
			s_Data->CombinationPipeline.ClearColors(clearInfo->color);
			s_Data->CombinationPipeline.EndRenderPass();
		}

		StartNewBatch();
		Stats->Reset();
	}

	void Renderer2D::EndScene()
	{
		Flush();
		s_Data->CombinationPipeline.EndCommandBuffer();
		s_Data->DeferredPipeline.EndCommandBuffer();
		//s_Data->TextPipeline.EndCommandBuffer();
	}

	void Renderer2D::SubmitSprite(const glm::vec3& worldPos, const glm::vec2& scale, float rotation, uint32_t layerIndex, const Texture* texture)
	{

	}

	void Renderer2D::SubmitQuad(const glm::vec3& worldPos, const glm::vec2& scale, const glm::vec4& color, float rotation, uint32_t layerIndex)
	{

	}

	void Renderer2D::SubmitText(const glm::vec3& pos, const glm::vec2& scale, const Texture* texture, const glm::vec4& color)
	{

	}

	void Renderer2D::SubmitLight2D(const glm::vec3& worldPos, const glm::vec4& color, float radius, float lightIntensity)
	{

	}

	void Renderer2D::Flush()
	{

	}

	void Renderer2D::StartNewBatch()
	{

	}

	void Renderer2D::CreatePipelines()
	{
		GraphicsPipelineCreateInfo pipelineCI = {};

		// Deferred pipeline
		{
			BufferLayout PBRlayout =
			{
				{ DataTypes::Float3, "aPos" },
				{ DataTypes::Float3, "aNormal" },
				{ DataTypes::Float4, "aTangent" },
				{ DataTypes::Float2, "aUV" },
				{ DataTypes::Int4,   "aBoneIDs"},
				{ DataTypes::Float4, "aWeight"}
			};

			GraphicsPipelineShaderCreateInfo shaderCI = {};
			{
				shaderCI.FilePaths[ShaderType::Vertex] = GraphicsContext::s_Instance->m_ResourcesFolderPath + "Shaders/Vulkan/2D_MRT.vert";
				shaderCI.FilePaths[ShaderType::Fragment] = GraphicsContext::s_Instance->m_ResourcesFolderPath + "Shaders/Vulkan/2D_MRT.frag";

				shaderCI.StorageBuffersSizes[1] = { sizeof(s_Data->ShaderInstanceSize) * Renderer2DStorage::MaxQuads };
			};

			pipelineCI.PipelineName = "Deferred_2D";
			pipelineCI.VertexInputInfos = { VertexInputInfo(sizeof(PBRVertex), PBRlayout) };
			pipelineCI.PipelineCullMode = CullMode::None;
			pipelineCI.bDepthTestEnabled = false;
			pipelineCI.TargetFramebuffer = &s_Data->DeferredFB;
			pipelineCI.ShaderCreateInfo = &shaderCI;

			assert(s_Data->DeferredPipeline.Create(&pipelineCI) == PipelineCreateResult::SUCCESS);
		}

		// Combination pipeline
		{
			BufferLayout Fullscreenlayout =
			{
				{ DataTypes::Float2, "aPos" },
				{ DataTypes::Float2, "aUV" },
			};

			GraphicsPipelineShaderCreateInfo shaderCI = {};
			{
				shaderCI.FilePaths[ShaderType::Vertex] = GraphicsContext::s_Instance->m_ResourcesFolderPath + "Shaders/Vulkan/GenVertex.vert";
				shaderCI.FilePaths[ShaderType::Fragment] = GraphicsContext::s_Instance->m_ResourcesFolderPath + "Shaders/Vulkan/2D_Combination.frag";

				shaderCI.StorageBuffersSizes[2] = { sizeof(PointLightBuffer) * Renderer2DStorage::Light2DBufferMaxSize };
			};

			pipelineCI.PipelineName = "2D_Combination";
			pipelineCI.PipelineCullMode = CullMode::Back;
			pipelineCI.VertexInputInfos = { VertexInputInfo(sizeof(FullscreenVertex), Fullscreenlayout) };
			pipelineCI.TargetFramebuffer = s_Data->MainFB;
			pipelineCI.ShaderCreateInfo = &shaderCI;

			assert(s_Data->CombinationPipeline.Create(&pipelineCI) == PipelineCreateResult::SUCCESS);
#ifndef FROSTIUM_OPENGL_IMPL
			auto& vkFB = s_Data->DeferredFB.GetVulkanFramebuffer();

			s_Data->CombinationPipeline.UpdateVulkanImageDescriptor(5, vkFB.GetAttachment(std::string("Albedro"))->ImageInfo);
			s_Data->CombinationPipeline.UpdateVulkanImageDescriptor(6, vkFB.GetAttachment(std::string("Position"))->ImageInfo);
			s_Data->CombinationPipeline.UpdateVulkanImageDescriptor(7, vkFB.GetAttachment(std::string("Normals"))->ImageInfo);
#endif

		}
	}

	void Renderer2D::CreateFramebuffers()
	{
		// Main framebuffer
		s_Data->MainFB = GraphicsContext::GetSingleton()->GetFramebuffer();
		// Deferred framebuffer
		FramebufferSpecification framebufferCI = {};
		{
			framebufferCI.Width = 2048;
			framebufferCI.Height = 2048;
			framebufferCI.bResizable = false;

			framebufferCI.Attachments.resize(3);
			framebufferCI.Attachments[0] = FramebufferAttachment(AttachmentFormat::SFloat4_32, true, "Albedro");
			framebufferCI.Attachments[1] = FramebufferAttachment(AttachmentFormat::SFloat4_32, true, "Position");
			framebufferCI.Attachments[2] = FramebufferAttachment(AttachmentFormat::SFloat4_32, true, "Normals");

			Framebuffer::Create(framebufferCI, &s_Data->DeferredFB);
		}
	}

}