#include "stdafx.h"
#include "Renderer/Renderer2D.h"
#include "Renderer/Renderer2DStorage.h"

#include "Primitives/GraphicsPipeline.h"
#include "Primitives/Mesh.h"
#include "Primitives/Text.h"

#include "Utils/Utils.h"

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	static Renderer2DStorage* s_Data;

	void Renderer2D::Init(Renderer2DStorage* storage)
	{
		if (storage == nullptr)
			std::runtime_error("Renderer: storage is nullptr!");

		Stats = new Renderer2DStats();
		s_Data = storage;
		s_Data->SceneData = &GraphicsContext::GetSingleton()->m_SceneData;

		CreateFramebuffers();
		CreatePipelines();
		Prepare();
	}

	void Renderer2D::Shutdown()
	{
		if (s_Data != nullptr)
		{
			delete Stats;
		}
	}

	void Renderer2D::BeginScene(const ClearInfo* clearInfo)
	{
		s_Data->MainPipeline.BeginCommandBuffer(true);
		s_Data->TextPipeline.BeginCommandBuffer(true);

		if (clearInfo->bClear)
		{
			s_Data->MainPipeline.BeginRenderPass();
			s_Data->MainPipeline.ClearColors(clearInfo->color);
			s_Data->MainPipeline.EndRenderPass();
		}

		StartNewBatch();
		Reset();
	}

	void Renderer2D::EndScene()
	{
		Flush();
		s_Data->MainPipeline.EndCommandBuffer();
		s_Data->TextPipeline.EndCommandBuffer();
	}

	void Renderer2D::SubmitSprite(const glm::vec3& worldPos, const glm::vec3& scale, const glm::vec3& rotation, uint32_t layerIndex, Texture* texture, const glm::vec4& color, GraphicsPipeline* material)
	{
		if (s_Data->Frustum->CheckSphere(worldPos, 10.0f))
		{
			if (s_Data->InstIndex >= Renderer2DStorage::MaxQuads)
				StartNewBatch();

			uint32_t index = s_Data->InstIndex;
			s_Data->Instances[index].Layer = layerIndex > Renderer2DStorage::MaxLayers ?
				const_cast<uint32_t*>(&Renderer2DStorage::MaxLayers) : &layerIndex;

			s_Data->Instances[index].Color = const_cast<glm::vec4*>(&color);
			s_Data->Instances[index].Position = const_cast<glm::vec3*>(&worldPos);
			s_Data->Instances[index].Rotation = const_cast<glm::vec3*>(&rotation);
			s_Data->Instances[index].Scale = const_cast<glm::vec3*>(&scale);
			s_Data->Instances[index].TextureIndex = AddTexture(texture);

			s_Data->InstIndex++;
		}
	}

	void Renderer2D::SubmitQuad(const glm::vec3& worldPos, const glm::vec3& scale, const glm::vec3& rotation, uint32_t layerIndex, const glm::vec4& color, GraphicsPipeline* material)
	{
		if (s_Data->Frustum->CheckSphere(worldPos, 10.0f))
		{
			if (s_Data->InstIndex >= Renderer2DStorage::MaxQuads)
				StartNewBatch();

			uint32_t index = s_Data->InstIndex;

			s_Data->Instances[index].Layer = layerIndex > Renderer2DStorage::MaxLayers ?
				const_cast<uint32_t*>(&Renderer2DStorage::MaxLayers) : &layerIndex;
			s_Data->Instances[index].Color = const_cast<glm::vec4*>(&color);
			s_Data->Instances[index].Position = const_cast<glm::vec3*>(&worldPos);
			s_Data->Instances[index].Rotation = const_cast<glm::vec3*>(&rotation);
			s_Data->Instances[index].Scale = const_cast<glm::vec3*>(&scale);
			s_Data->Instances[index].TextureIndex = 0;

			s_Data->InstIndex++;
		}
	}

	void Renderer2D::SubmitText(Text* text)
	{
		auto& msg = s_Data->TextMessages[s_Data->TextIndex];
		msg.Obj = text;
		msg.TextureIndex = s_Data->TextIndex;
		s_Data->FontTextures[s_Data->TextIndex] = &text->m_SDFTexture;
		Utils::ComposeTransform(text->m_Pos, text->m_Rotation, text->m_Scale, msg.Model);

		s_Data->TextIndex++;
	}

	void Renderer2D::SubmitLight2D(const glm::vec3& worldPos, const glm::vec4& color, float radius, float lightIntensity)
	{
		if (s_Data->Frustum->CheckSphere(worldPos, 15.0f))
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
				if (l == *inst.Layer)
				{
					auto& cmd = s_Data->CommandBuffer[l];
					if (cmd.OffsetApplied == false)
					{
						cmd.DataOffset = i;
						cmd.OffsetApplied = true;
					}

					cmd.Instances++;
					s_Data->ShaderInstances[i].Color = *inst.Color;
					s_Data->ShaderInstances[i].Params.w = *inst.Layer;
					s_Data->ShaderInstances[i].Params.x = inst.TextureIndex;
					Utils::ComposeTransform2D(*inst.Position, *inst.Rotation, *inst.Scale, s_Data->ShaderInstances[i].Model);
				}
			}
		}

		if (s_Data->InstIndex > 0 || s_Data->TextIndex > 0)
		{
			// Update Scene data
			s_Data->MainPipeline.SubmitBuffer(s_Data->SceneDataBP, sizeof(SceneData), s_Data->SceneData);
			// Update instances data
			s_Data->MainPipeline.SubmitBuffer(s_Data->InstancesBP, s_Data->ShaderInstanceSize * s_Data->InstIndex, s_Data->ShaderInstances);
			// Updates tetxures
			s_Data->MainPipeline.UpdateSamplers(s_Data->Textures, 0);
			// Updates font tetxures
			s_Data->TextPipeline.UpdateSamplers(s_Data->FontTextures, 1);

			s_Data->MainPipeline.BeginRenderPass();
			{
				// Sprites
				for (uint32_t i = 0; i < Renderer2DStorage::MaxLayers; ++i)
				{
					auto& cmd = s_Data->CommandBuffer[i];
					if (cmd.Instances > 0)
					{
						s_Data->MainPipeline.SubmitPushConstant(ShaderType::Vertex, sizeof(uint32_t), &cmd.DataOffset);
						s_Data->MainPipeline.DrawMeshIndexed(&s_Data->PlaneMesh, cmd.Instances);

						cmd.Reset();
					}
				}

				// Text
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
			s_Data->MainPipeline.EndRenderPass();
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

		// Main pipeline
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
				shaderCI.FilePaths[ShaderType::Vertex] = GraphicsContext::s_Instance->m_ResourcesFolderPath + "Shaders/2D.vert";
				shaderCI.FilePaths[ShaderType::Fragment] = GraphicsContext::s_Instance->m_ResourcesFolderPath + "Shaders/2D.frag";

				ShaderBufferInfo bufferInfo = {};
				bufferInfo.bGlobal = false;
				bufferInfo.Size = sizeof(s_Data->ShaderInstanceSize) * Renderer2DStorage::MaxQuads;

				shaderCI.BufferInfos[1] = bufferInfo;
			};

			pipelineCI.bDepthWriteEnabled = false;
			pipelineCI.bDepthTestEnabled = false;
			pipelineCI.PipelineName = "Deferred_2D";
			pipelineCI.VertexInputInfos = { VertexInputInfo(sizeof(PBRVertex), PBRlayout) };
			pipelineCI.eCullMode = CullMode::None;
			pipelineCI.eSrcColorBlendFactor = BlendFactor::SRC_ALPHA;
			pipelineCI.eDstColorBlendFactor = BlendFactor::ONE_MINUS_SRC_ALPHA;
			pipelineCI.eSrcAlphaBlendFactor = BlendFactor::SRC_ALPHA;
			pipelineCI.eDstAlphaBlendFactor = BlendFactor::ONE_MINUS_SRC_ALPHA;
			pipelineCI.eColorBlendOp = BlendOp::ADD;
			pipelineCI.eAlphaBlendOp = BlendOp::SUBTRACT;

			pipelineCI.TargetFramebuffers = { s_Data->MainFB };
			pipelineCI.ShaderCreateInfo = shaderCI;

			assert(s_Data->MainPipeline.Create(&pipelineCI) == PipelineCreateResult::SUCCESS);
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
				shaderCI.FilePaths[ShaderType::Vertex] = GraphicsContext::s_Instance->m_ResourcesFolderPath + "Shaders/Text.vert";
				shaderCI.FilePaths[ShaderType::Fragment] = GraphicsContext::s_Instance->m_ResourcesFolderPath + "Shaders/Text.frag";
			};

			pipelineCI.PipelineName = "Text";
			pipelineCI.eCullMode = CullMode::None;
			pipelineCI.bDepthTestEnabled = false;
			pipelineCI.eSrcColorBlendFactor = BlendFactor::ONE;
			pipelineCI.eDstColorBlendFactor = BlendFactor::ONE_MINUS_SRC_ALPHA;
			pipelineCI.eSrcAlphaBlendFactor = BlendFactor::ONE;
			pipelineCI.eDstAlphaBlendFactor = BlendFactor::ZERO;
			pipelineCI.VertexInputInfos = { VertexInputInfo(sizeof(TextVertex), layout) };
			pipelineCI.TargetFramebuffers = { s_Data->MainFB };
			pipelineCI.ShaderCreateInfo = shaderCI;

			assert(s_Data->TextPipeline.Create(&pipelineCI) == PipelineCreateResult::SUCCESS);
		}
	}

	void Renderer2D::CreateFramebuffers()
	{
		// Main framebuffer
		s_Data->MainFB = GraphicsContext::GetSingleton()->GetFramebuffer();
	}

	void Renderer2D::Prepare()
	{
		s_Data->WhiteTetxure = GraphicsContext::s_Instance->m_DummyTexure;
		s_Data->Textures[0] = s_Data->WhiteTetxure;

		Mesh::Create(GraphicsContext::s_Instance->m_ResourcesFolderPath + "Models/plane.gltf", &s_Data->PlaneMesh);
	}
}