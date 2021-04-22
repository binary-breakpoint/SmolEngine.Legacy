#include "stdafx.h"
#include "Renderer2D.h"
#include "GraphicsPipeline.h"

#include "Common/Mesh.h"
#include "Common/Text.h"
#include "Utils/Utils.h"

namespace Frostium
{
	struct Instance
	{
		uint32_t  Layer = 0;
		uint32_t  TextureIndex = 0;

		glm::vec3 Position = glm::vec3(1.0f);
		glm::vec3 Rotation = glm::vec3(0.0f);
		glm::vec3 Scale = glm::vec3(1.0f);
		glm::vec4 Color = glm::vec4(1.0f);
	};

	struct CmdBuffer
	{
		bool      OffsetApplied = false;
		uint32_t  Instances = 0;
		uint32_t  DataOffset = 0;

		void Reset()
		{
			DataOffset = 0;
			Instances = 0;
			OffsetApplied = 0;
		}
	};

	struct ShaderInstance
	{
		glm::mat4  Model;
		glm::vec4  Color;
		glm::ivec4 Params; // x - texture index
	};

	struct TextBuffer
	{
		glm::mat4    Model;
		Text*        Obj = nullptr;
		uint32_t     TextureIndex = 0;
	};				 

	struct Renderer2DStorage
	{
		Renderer2DStorage()
		{
			Frustum = GraphicsContext::GetSingleton()->GetFrustum();
			Textures.resize(Renderer2D::MaxTextureSlot);
			FontTextures.resize(MaxTextMessages);
		}

		~Renderer2DStorage() { }

		// Limits
		static const uint32_t     Light2DBufferMaxSize = 100;
		static const uint32_t     MaxQuads = 15000;
		static const uint32_t     MaxTextMessages = 100;
		static const uint32_t     MaxLayers = 12;
		// Count tracking
		uint32_t                  InstIndex = 0;
		uint32_t                  TextIndex = 1; 
		uint32_t                  SampleIndex = 1; // 0 - white dummy texture
		uint32_t                  FontSampleIndex = 0; 
		// Framebuffers
		Framebuffer               DeferredFB = {};
		Framebuffer*              MainFB = nullptr;
		// Pipelines
		GraphicsPipeline          DeferredPipeline = {};
		GraphicsPipeline          CombinationPipeline = {};
		GraphicsPipeline          TextPipeline = {};
		// Refs
		Frustum*                  Frustum = nullptr;
		Texture*                  WhiteTetxure = nullptr;
		// Meshes
		Mesh                      PlaneMesh = {};
		// UBO's and SSBO's
		SceneData                 SceneData = {};
		ShaderInstance            ShaderInstances[MaxQuads];
		TextBuffer                TextMessages[MaxTextMessages];
		// Buffers
		Instance                  Instances[MaxQuads];
		CmdBuffer                 CommandBuffer[MaxLayers];
		std::vector<Texture*>     Textures; 
		std::vector<Texture*>     FontTextures;
		// Bindings
		const uint32_t            InstancesBP = 1;
		const uint32_t            SamplersBP = 2;
		const uint32_t            SceneDataBP = 27;
		// Push Constants
		struct TextPC
		{
			glm::mat4             Model;
			glm::vec4             Color;
			uint32_t              TexIndex;
		};
		TextPC                    TextPushConstant;
		// Sizes
		const size_t              ShaderInstanceSize = sizeof(ShaderInstance);
		const size_t              TextPCSize = sizeof(TextPC);
	};

	static Renderer2DStorage* s_Data;

	void Renderer2D::Init()
	{
		Stats = new Renderer2DStats();
		s_Data = new Renderer2DStorage();

		CreateFramebuffers();
		CreatePipelines();
		Prepare();
	}

	void Renderer2D::Shutdown()
	{
		if (s_Data != nullptr)
		{
			delete s_Data, Stats;
		}
	}

	void Renderer2D::BeginScene(const ClearInfo* clearInfo, const BeginSceneInfo* info)
	{
		if (GraphicsContext::GetSingleton()->m_State->UseEditorCamera)
		{
			Camera* camera = GraphicsContext::GetSingleton()->GetDefaultCamera();
			s_Data->SceneData.View = camera->GetViewMatrix();
			s_Data->SceneData.Projection = camera->GetProjection();
		}

		if (info)
		{
			s_Data->SceneData.View = info->view;
			s_Data->SceneData.Projection = info->proj;
		}

		s_Data->Frustum->Update(s_Data->SceneData.Projection * s_Data->SceneData.View);
		s_Data->CombinationPipeline.BeginCommandBuffer(true);
		s_Data->DeferredPipeline.BeginCommandBuffer(true);
		s_Data->TextPipeline.BeginCommandBuffer(true);

		if (clearInfo->bClear)
		{
			s_Data->CombinationPipeline.BeginRenderPass();
			s_Data->CombinationPipeline.ClearColors(clearInfo->color);
			s_Data->CombinationPipeline.EndRenderPass();
		}

		if (s_Data->InstIndex > 0)
		{
			s_Data->DeferredPipeline.BeginRenderPass();
			s_Data->DeferredPipeline.ClearColors(clearInfo->color);
			s_Data->DeferredPipeline.EndRenderPass();
		}

		StartNewBatch();
		Reset();
	}

	void Renderer2D::EndScene()
	{
		Flush();
		s_Data->CombinationPipeline.EndCommandBuffer();
		s_Data->DeferredPipeline.EndCommandBuffer();
		s_Data->TextPipeline.EndCommandBuffer();
	}

	void Renderer2D::SubmitSprite(const glm::vec2& worldPos, const glm::vec2& scale, float rotation, uint32_t layerIndex, Texture* texture, const glm::vec4& color, GraphicsPipeline* material)
	{
		if (s_Data->Frustum->CheckSphere(glm::vec3(worldPos, 0.0f), 10.0f))
		{
			if (s_Data->InstIndex >= Renderer2DStorage::MaxQuads)
				StartNewBatch();

			uint32_t index = s_Data->InstIndex;

			s_Data->Instances[index].Layer = layerIndex > Renderer2DStorage::MaxLayers ? Renderer2DStorage::MaxLayers : layerIndex;
			s_Data->Instances[index].Color = color;
			s_Data->Instances[index].Position = { worldPos, 0.0f };
			s_Data->Instances[index].Rotation = { rotation, rotation, 0 };
			s_Data->Instances[index].Scale = { scale, 1 };
			s_Data->Instances[index].TextureIndex = AddTexture(texture);

			s_Data->InstIndex++;
		}
	}

	void Renderer2D::SubmitQuad(const glm::vec2& worldPos, const glm::vec2& scale, float rotation, uint32_t layerIndex, const glm::vec4& color, GraphicsPipeline* material)
	{
		if (s_Data->Frustum->CheckSphere(glm::vec3(worldPos, 0.0f), 10.0f))
		{
			if (s_Data->InstIndex >= Renderer2DStorage::MaxQuads)
				StartNewBatch();

			uint32_t index = s_Data->InstIndex;

			s_Data->Instances[index].Layer = layerIndex > Renderer2DStorage::MaxLayers ? Renderer2DStorage::MaxLayers : layerIndex;
			s_Data->Instances[index].Color = color;
			s_Data->Instances[index].Position = { worldPos, 0.0f };
			s_Data->Instances[index].Rotation = { rotation, rotation, 0 };
			s_Data->Instances[index].Scale = { scale, 1 };
			s_Data->Instances[index].TextureIndex = 0;

			s_Data->InstIndex++;
		}
	}

	void Renderer2D::SubmitText(Text* text)
	{
		auto& msg = s_Data->TextMessages[s_Data->TextIndex];
		msg.Obj = text;
		msg.TextureIndex = 0;
		s_Data->FontTextures[0] = &text->m_SDFTexture;
		Utils::ComposeTransform(text->m_Pos, text->m_Rotation, text->m_Scale, false, msg.Model);

		s_Data->TextIndex++;
	}

	void Renderer2D::SubmitLight2D(const glm::vec2& worldPos, const glm::vec4& color, float radius, float lightIntensity)
	{
		if (s_Data->Frustum->CheckSphere(glm::vec3(worldPos, 0.0f), 15.0f))
		{

		}
	}

	void Renderer2D::Flush()
	{
		// Fill command buffer
		for (uint32_t i = 0; i < s_Data->InstIndex; ++i)
		{
			for (uint32_t l = 0; l < Renderer2DStorage::MaxLayers; ++l)
			{
				Instance& inst = s_Data->Instances[i];
				if (l == inst.Layer)
				{
					auto& cmd = s_Data->CommandBuffer[l];
					if (cmd.OffsetApplied == false)
					{
						cmd.DataOffset = i;
						cmd.OffsetApplied = true;
					}

					cmd.Instances++;
					s_Data->ShaderInstances[i].Color = inst.Color;
					s_Data->ShaderInstances[i].Params.x = inst.TextureIndex;
					s_Data->ShaderInstances[i].Params.w = inst.Layer;
					Utils::ComposeTransform(inst.Position, inst.Rotation, inst.Scale, false, s_Data->ShaderInstances[i].Model);
				}
			}
		}

		// Update Scene data
		s_Data->DeferredPipeline.SubmitBuffer(s_Data->SceneDataBP, sizeof(SceneData), &s_Data->SceneData);

		if (s_Data->InstIndex > 0)
		{
			// Update instances data
			s_Data->DeferredPipeline.SubmitBuffer(s_Data->InstancesBP, s_Data->ShaderInstanceSize * s_Data->InstIndex, s_Data->ShaderInstances);
			// Updates tetxures
			s_Data->DeferredPipeline.UpdateSamplers(s_Data->Textures, 0);

			// Deferred Pass
			{
				s_Data->DeferredPipeline.BeginRenderPass();
				{
					for (uint32_t i = 0; i < Renderer2DStorage::MaxLayers; ++i)
					{
						auto& cmd = s_Data->CommandBuffer[i];
						if (cmd.Instances > 0)
						{
							s_Data->DeferredPipeline.SubmitPushConstant(ShaderType::Vertex, sizeof(uint32_t), &cmd.DataOffset);
							s_Data->DeferredPipeline.DrawMesh(&s_Data->PlaneMesh, cmd.Instances);

							cmd.Reset();
						}
					}
				}
				s_Data->DeferredPipeline.EndRenderPass();
			}

			// Combination Pass (Lighting + Post-Processing)
			{
				s_Data->CombinationPipeline.BeginRenderPass();
				{
					s_Data->CombinationPipeline.DrawIndexed();
				}
				s_Data->CombinationPipeline.EndRenderPass();
			}
		}

		if (s_Data->TextIndex > 0)
		{
			s_Data->TextPipeline.UpdateSamplers(s_Data->FontTextures, 1);
			s_Data->TextPipeline.BeginRenderPass();
			{
				for (uint32_t i = 0; i < s_Data->TextIndex; ++i)
				{
					auto& msg = s_Data->TextMessages[i];

					s_Data->TextPushConstant.Model = msg.Model;
					s_Data->TextPushConstant.Color = msg.Obj->m_Color;
					s_Data->TextPushConstant.TexIndex = msg.TextureIndex;

					s_Data->TextPipeline.SubmitPushConstant(ShaderType::Vertex, s_Data->TextPCSize, &s_Data->TextPushConstant);
					s_Data->TextPipeline.DrawIndexed(&msg.Obj->m_VertexBuffer, &msg.Obj->m_IndexBuffer);
				}
			}
			s_Data->TextPipeline.EndRenderPass();
		}
	}

	void Renderer2D::StartNewBatch()
	{

	}

	void Renderer2D::Reset()
	{
		s_Data->InstIndex = 0;
		s_Data->SampleIndex = 1;
		s_Data->TextIndex = 0;
		s_Data->FontSampleIndex = 0;

		Stats->Reset();
	}

	uint32_t Renderer2D::AddTexture(Texture* tex_)
	{
		for (uint32_t i = 0; i < s_Data->SampleIndex; ++i)
		{
			Texture* tex = s_Data->Textures[i];
			if (tex == tex_)
				return i;
		}

		uint32_t index = s_Data->SampleIndex;
		s_Data->Textures[index] = tex_;
		s_Data->SampleIndex++;
		return index;
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
				{ DataTypes::Float3, "aTangent" },
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
			pipelineCI.eCullMode = CullMode::Front;
			pipelineCI.bDepthTestEnabled = false;
			pipelineCI.pTargetFramebuffer = &s_Data->DeferredFB;
			pipelineCI.pShaderCreateInfo = &shaderCI;

			assert(s_Data->DeferredPipeline.Create(&pipelineCI) == PipelineCreateResult::SUCCESS);
		}

		// Combination pipeline
		{
			float quadVertices[] = {
				// positions   // texCoords
				-1.0f, -1.0f,  0.0f, 1.0f,
				 1.0f, -1.0f,  1.0f, 1.0f,
				 1.0f,  1.0f,  1.0f, 0.0f,
				-1.0f,  1.0f,  0.0f, 0.0f
			};

			BufferLayout Fullscreenlayout =
			{
				{ DataTypes::Float2, "aPos" },
				{ DataTypes::Float2, "aUV" },
			};

			uint32_t squareIndices[6] = { 0, 1, 2, 2, 3, 0 };
			Ref<VertexBuffer> vb = std::make_shared<VertexBuffer>();
			Ref<IndexBuffer> ib = std::make_shared<IndexBuffer>();
			VertexBuffer::Create(vb.get(), quadVertices, sizeof(quadVertices));
			IndexBuffer::Create(ib.get(), squareIndices, 6);

			GraphicsPipelineShaderCreateInfo shaderCI = {};
			{
				shaderCI.FilePaths[ShaderType::Vertex] = GraphicsContext::s_Instance->m_ResourcesFolderPath + "Shaders/Vulkan/GenVertex.vert";
				shaderCI.FilePaths[ShaderType::Fragment] = GraphicsContext::s_Instance->m_ResourcesFolderPath + "Shaders/Vulkan/2D_Combination.frag";

				shaderCI.StorageBuffersSizes[2] = { sizeof(PointLightBuffer) * Renderer2DStorage::Light2DBufferMaxSize };
			};

			pipelineCI.PipelineName = "2D_Combination";
			pipelineCI.eCullMode = CullMode::Back;
			pipelineCI.VertexInputInfos = { VertexInputInfo(sizeof(FullscreenVertex), Fullscreenlayout) };
			pipelineCI.pTargetFramebuffer = s_Data->MainFB;
			pipelineCI.pShaderCreateInfo = &shaderCI;

			assert(s_Data->CombinationPipeline.Create(&pipelineCI) == PipelineCreateResult::SUCCESS);

			s_Data->CombinationPipeline.SetVertexBuffers({ vb });
			s_Data->CombinationPipeline.SetIndexBuffers({ ib });
			s_Data->CombinationPipeline.UpdateSampler(&s_Data->DeferredFB, 5, "Albedro");
			s_Data->CombinationPipeline.UpdateSampler(&s_Data->DeferredFB, 6, "Position");
			s_Data->CombinationPipeline.UpdateSampler(&s_Data->DeferredFB, 7, "Normals");
		}

		// Text Pipeline
		{
			pipelineCI = {};
			BufferLayout layout =
			{
				{ DataTypes::Float3, "aPos" },
				{ DataTypes::Float2, "aUV" },
			};

			GraphicsPipelineShaderCreateInfo shaderCI = {};
			{
				shaderCI.FilePaths[ShaderType::Vertex] = GraphicsContext::s_Instance->m_ResourcesFolderPath + "Shaders/Vulkan/Text.vert";
				shaderCI.FilePaths[ShaderType::Fragment] = GraphicsContext::s_Instance->m_ResourcesFolderPath + "Shaders/Vulkan/Text.frag";
			};

			pipelineCI.PipelineName = "Text";
			pipelineCI.eCullMode = CullMode::None;
			pipelineCI.eSrcColorBlendFactor = BlendFactor::ONE;
			pipelineCI.eDstColorBlendFactor = BlendFactor::ONE_MINUS_SRC_ALPHA;
			pipelineCI.eSrcAlphaBlendFactor = BlendFactor::ONE;
			pipelineCI.eDstAlphaBlendFactor = BlendFactor::ZERO;
			pipelineCI.VertexInputInfos = { VertexInputInfo(sizeof(TextVertex), layout) };
			pipelineCI.pTargetFramebuffer = s_Data->MainFB;
			pipelineCI.pShaderCreateInfo = &shaderCI;

			assert(s_Data->TextPipeline.Create(&pipelineCI) == PipelineCreateResult::SUCCESS);
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
			framebufferCI.eMSAASampels = MSAASamples::SAMPLE_COUNT_1;

			framebufferCI.Attachments.resize(3);
			framebufferCI.Attachments[0] = FramebufferAttachment(AttachmentFormat::Color, false, "Albedro");
			framebufferCI.Attachments[1] = FramebufferAttachment(AttachmentFormat::SFloat4_32, false, "Position");
			framebufferCI.Attachments[2] = FramebufferAttachment(AttachmentFormat::SFloat4_32, false, "Normals");

			Framebuffer::Create(framebufferCI, &s_Data->DeferredFB);
		}
	}

	void Renderer2D::Prepare()
	{
		s_Data->WhiteTetxure = GraphicsContext::s_Instance->m_DummyTexure;
		s_Data->Textures[0] = s_Data->WhiteTetxure;

		Mesh::Create(GraphicsContext::s_Instance->m_ResourcesFolderPath + "Models/plane.gltf", &s_Data->PlaneMesh);
	}
}