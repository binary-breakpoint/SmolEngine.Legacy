#include "stdafx.h"
#include "Renderer/Renderer2D.h"

#include "Primitives/Mesh.h"
#include "Tools/Utils.h"

#ifdef OPENGL_IMPL
#else
#include "Backends/Vulkan/VulkanPipeline.h"
#endif

#include <imgui/imgui.h>

namespace SmolEngine
{
	void Renderer2DStorage::Initilize()
	{
		CreateFramebuffers();
		CreatePipelines();
	}

	Renderer2DStorage::Renderer2DStorage()
	{
		s_Instance = this;
	}

	Renderer2DStorage::~Renderer2DStorage()
	{
		s_Instance = nullptr;
	}

	void Renderer2DStorage::SetRenderTarget(Ref<Framebuffer>& target)
	{
		s_Instance->MainPipeline->SetFramebuffers({ target });
		s_Instance->MainFB = target;
	}

	void Renderer2DStorage::CreatePipelines()
	{
		const std::string& path = GraphicsContext::GetSingleton()->GetResourcesPath();
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

			ShaderCreateInfo shaderCI = {};
			{
				shaderCI.Stages[ShaderType::Vertex] = path + "Shaders/2D.vert";
				shaderCI.Stages[ShaderType::Fragment] = path + "Shaders/2D.frag";

				ShaderBufferInfo bufferInfo = {};
				bufferInfo.bGlobal = false;
				bufferInfo.Size = sizeof(ShaderInstanceSize) * RendererDrawList2D::MaxQuads;

				shaderCI.Buffers[1] = bufferInfo;
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

			pipelineCI.TargetFramebuffers = { MainFB };
			pipelineCI.ShaderCreateInfo = shaderCI;

			MainPipeline = GraphicsPipeline::Create();
			assert(MainPipeline->Build(&pipelineCI) == true);

		}

		PlaneMesh = Mesh::Create();
		PlaneMesh->LoadFromFile("Models/plane.gltf");
	}

	void Renderer2DStorage::CreateFramebuffers()
	{
		MainFB = GraphicsContext::GetSingleton()->GetMainFramebuffer();
	}

	void Renderer2DStorage::UpdateUniforms(RendererDrawList2D* drawList)
	{
		MainPipeline->SubmitBuffer(SceneDataBP, sizeof(SceneViewProjection), drawList->SceneInfo);
		MainPipeline->SubmitBuffer(InstancesBP, ShaderInstanceSize * drawList->InstIndex, drawList->ShaderInstances);
		MainPipeline->UpdateSamplers(drawList->Textures, 0);
	}

	void Renderer2D::DrawFrame(ClearInfo* clearInfo, bool batch_cmd)
	{
		CommandBufferStorage cmdBuffer;
		Renderer2DStorage* storage = Renderer2DStorage::GetSingleton();
		RendererDrawList2D* drawList = RendererDrawList2D::GetSingleton();

		PrepareCmdBuffer(&cmdBuffer, storage, batch_cmd);
		ClearAtachments(clearInfo, storage);
		UpdateUniforms(storage, drawList);
		UpdateCmdBuffer(storage, drawList);

		if (!batch_cmd) { VulkanCommandBuffer::ExecuteCommandBuffer(&cmdBuffer); }
	}

	void Renderer2D::UpdateCmdBuffer(Renderer2DStorage* storage, RendererDrawList2D* drawList)
	{
		if (drawList->InstIndex > 0 || drawList->TextIndex > 0)
		{
			storage->MainPipeline->BeginRenderPass();
			{
				for (uint32_t i = 0; i < RendererDrawList2D::MaxLayers; ++i)
				{
					auto& cmd = drawList->CommandBuffer[i];
					if (cmd.Instances > 0)
					{
						storage->MainPipeline->SubmitPushConstant(ShaderType::Vertex, sizeof(uint32_t), &cmd.DataOffset);
						storage->MainPipeline->DrawMeshIndexed(storage->PlaneMesh, cmd.Instances);

						cmd.Reset();
					}
				}
			}
			storage->MainPipeline->EndRenderPass();
		}
	}

	void Renderer2D::ClearAtachments(const ClearInfo* clearInfo, Renderer2DStorage* storage)
	{
		if (clearInfo->bClear)
		{
			storage->MainPipeline->BeginRenderPass();
			storage->MainPipeline->ClearColors(clearInfo->color);
			storage->MainPipeline->EndRenderPass();
		}
	}

	void Renderer2D::UpdateUniforms(Renderer2DStorage* storage, RendererDrawList2D* drawList)
	{
		storage->UpdateUniforms(drawList);
	}

	void Renderer2D::PrepareCmdBuffer(CommandBufferStorage* cmd, Renderer2DStorage* storage, bool batch_cmd)
	{
		if (batch_cmd)
		{
			storage->MainPipeline->BeginCommandBuffer(batch_cmd);
			return;
		}

		VulkanCommandBuffer::CreateCommandBuffer(cmd);
		storage->MainPipeline->Cast<VulkanPipeline>()->SetCommandBuffer(cmd->Buffer);
	}

	RendererDrawList2D::RendererDrawList2D()
	{
		Textures.resize(MaxTextureSlot);
		FontTextures.resize(MaxTextMessages);

		Textures[0] = TexturePool::GetWhiteTexture();
		s_Instance = this;
	}

	RendererDrawList2D::~RendererDrawList2D()
	{
		s_Instance = nullptr;
	}

	void RendererDrawList2D::BeginSubmit(SceneViewProjection* viewProj)
	{
		ClearDrawList();
		CalculateFrustum(viewProj);
	}

	void RendererDrawList2D::EndSubmit()
	{
		s_Instance->BuildDrawList();
	}

	void RendererDrawList2D::SubmitSprite(const glm::vec3& worldPos, const glm::vec3& scale, const glm::vec3& rotation, uint32_t layerIndex, Ref<Texture>& texture, bool frustumCulling, const glm::vec4& color)
	{
		if (!s_Instance->IsDrawable(worldPos, frustumCulling))
			return;

		uint32_t index = s_Instance->InstIndex;
		s_Instance->Instances[index].Layer = layerIndex > RendererDrawList2D::MaxLayers ? const_cast<uint32_t*>(&RendererDrawList2D::MaxLayers) : &layerIndex;
		s_Instance->Instances[index].Color = const_cast<glm::vec4*>(&color);
		s_Instance->Instances[index].Position = const_cast<glm::vec3*>(&worldPos);
		s_Instance->Instances[index].Rotation = const_cast<glm::vec3*>(&rotation);
		s_Instance->Instances[index].Scale = const_cast<glm::vec3*>(&scale);
		s_Instance->Instances[index].TextureIndex = s_Instance->AddTexture(texture);

		s_Instance->InstIndex++;
	}

	void RendererDrawList2D::SubmitQuad(const glm::vec3& worldPos, const glm::vec3& scale, const glm::vec3& rotation, uint32_t layerIndex, bool frustumCulling, const glm::vec4& color)
	{
		if (!s_Instance->IsDrawable(worldPos, frustumCulling))
			return;

		uint32_t index = s_Instance->InstIndex;
		s_Instance->Instances[index].Layer = layerIndex > RendererDrawList2D::MaxLayers ? const_cast<uint32_t*>(&RendererDrawList2D::MaxLayers) : &layerIndex;
		s_Instance->Instances[index].Color = const_cast<glm::vec4*>(&color);
		s_Instance->Instances[index].Position = const_cast<glm::vec3*>(&worldPos);
		s_Instance->Instances[index].Rotation = const_cast<glm::vec3*>(&rotation);
		s_Instance->Instances[index].Scale = const_cast<glm::vec3*>(&scale);
		s_Instance->Instances[index].TextureIndex = 0;

		s_Instance->InstIndex++;
	}

	void RendererDrawList2D::SubmitLight2D(const glm::vec3& worldPos, const glm::vec4& color, float radius, float lightIntensity, bool frustumCulling)
	{

	}

	uint32_t RendererDrawList2D::AddTexture(Ref<Texture>& in_texture)
	{
		// temp
		for (uint32_t i = 0; i < SampleIndex; ++i)
		{
			auto tex = Textures[i];
			if (tex == in_texture)
				return i;
		}

		uint32_t index = SampleIndex;
		Textures[index] = in_texture;
		SampleIndex++;
		return index;
	}

	void RendererDrawList2D::CalculateFrustum(SceneViewProjection* sceneViewProj)
	{
		s_Instance->SceneInfo = sceneViewProj;
		s_Instance->Frustum.Update(sceneViewProj->Projection * sceneViewProj->View);
	}

	Frustum& RendererDrawList2D::GetFrustum()
	{
		return s_Instance->Frustum;
	}

	void RendererDrawList2D::ClearDrawList()
	{
		s_Instance->InstIndex = 0;
		s_Instance->SampleIndex = 1;
		s_Instance->TextIndex = 0;
		s_Instance->FontSampleIndex = 0;
	}

	void RendererDrawList2D::BuildDrawList()
	{
		for (uint32_t i = 0; i < InstIndex; ++i)
		{
			for (uint32_t l = 0; l < RendererDrawList2D::MaxLayers; ++l)
			{
				Instance& inst = Instances[i];
				if (l == *inst.Layer)
				{
					auto& cmd = CommandBuffer[l];
					if (cmd.OffsetApplied == false)
					{
						cmd.DataOffset = i;
						cmd.OffsetApplied = true;
					}

					cmd.Instances++;
					ShaderInstances[i].Color = *inst.Color;
					ShaderInstances[i].Params.w = *inst.Layer;
					ShaderInstances[i].Params.x = inst.TextureIndex;
					Utils::ComposeTransform2D(*inst.Position, *inst.Rotation, *inst.Scale, ShaderInstances[i].Model);
				}
			}
		}
	}

	bool RendererDrawList2D::IsDrawable(const glm::vec3& worldPos, bool frustumCulling)
	{
		if (frustumCulling)
		{
			if (!Frustum.CheckSphere(worldPos)) { return false; }
		}

		if (InstIndex >= RendererDrawList2D::MaxQuads)
			return false;

		return true;
	}
}