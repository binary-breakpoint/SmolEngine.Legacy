#include "stdafx.h"
#include "DebugRenderer.h"

#include "Common/Framebuffer.h"
#include "Common/Mesh.h"
#include "Utils/Utils.h"
#include "GraphicsPipeline.h"

namespace Frostium
{
	struct DebugRendererStorage
	{
		GraphicsPipeline PrimitivePipeline{};
		GraphicsPipeline MeshPipeline{};
	};

	DebugRendererStorage* s_Data = nullptr;

	void DebugRenderer::BeginDebug()
	{
		s_Data->MeshPipeline.BeginCommandBuffer(true);
		s_Data->MeshPipeline.BeginCommandBuffer(true);

		s_Data->MeshPipeline.BeginRenderPass();
	}

	void DebugRenderer::EndDebug()
	{
		s_Data->MeshPipeline.EndRenderPass();
	}

	void DebugRenderer::DrawQuad(const glm::vec2& pos, const glm::vec2& rotation, const glm::vec2& scale)
	{

	}

	void DebugRenderer::DrawCirlce(const glm::vec2& pos, const glm::vec2& scale)
	{

	}

	void DebugRenderer::DrawMesh(const glm::vec3& pos, const glm::vec3& rotation, const glm::vec3& scale, Mesh* mesh)
	{
		glm::mat4 model;
		Utils::ComposeTransform(pos, rotation, scale, model);

		s_Data->MeshPipeline.SubmitPushConstant(ShaderType::Vertex, sizeof(glm::mat4), &model);
		s_Data->MeshPipeline.DrawMeshIndexed(mesh, 1);

		for(auto& child: mesh->GetMeshes())
			DrawMesh(pos, rotation, scale, const_cast<Mesh*>(&child));
	}

	void DebugRenderer::Init()
	{
		s_Data = new DebugRendererStorage();

		// Primitives
		{
			BufferLayout layout =
			{
				{ DataTypes::Float3, "aPos" }
			};

			GraphicsPipelineCreateInfo pipelineCI = {};
			GraphicsPipelineShaderCreateInfo shaderCI = {};
			{
				shaderCI.FilePaths[ShaderType::Vertex] = GraphicsContext::GetSingleton()->m_ResourcesFolderPath + "Shaders/Vulkan/DebugPrimitive.vert";
				shaderCI.FilePaths[ShaderType::Fragment] = GraphicsContext::GetSingleton()->m_ResourcesFolderPath + "Shaders/Vulkan/DebugPrimitive.frag";
			};

			pipelineCI.PipelineName = "DebugPrimitive";
			pipelineCI.eCullMode = CullMode::None;
			pipelineCI.VertexInputInfos = { VertexInputInfo(sizeof(glm::vec3), layout) };
			pipelineCI.PipelineDrawModes = { DrawMode::Line, DrawMode::Fan };
			pipelineCI.bDepthTestEnabled = false;
			pipelineCI.pTargetFramebuffer = GraphicsContext::GetSingleton()->GetFramebuffer();
			pipelineCI.pShaderCreateInfo = &shaderCI;

			assert(s_Data->PrimitivePipeline.Create(&pipelineCI) == PipelineCreateResult::SUCCESS);
		}

		// Mesh
		{

			BufferLayout layout =
			{
				{ DataTypes::Float3, "aPos" },
				{ DataTypes::Float3, "aNormal" },
				{ DataTypes::Float3, "aTangent" },
				{ DataTypes::Float2, "aUV" },
				{ DataTypes::Int4,   "aBoneIDs"},
				{ DataTypes::Float4, "aWeight"}
			};

			GraphicsPipelineCreateInfo pipelineCI = {};
			GraphicsPipelineShaderCreateInfo shaderCI = {};
			{
				shaderCI.FilePaths[ShaderType::Vertex] = GraphicsContext::GetSingleton()->m_ResourcesFolderPath + "Shaders/Vulkan/DebugMesh.vert";
				shaderCI.FilePaths[ShaderType::Fragment] = GraphicsContext::GetSingleton()->m_ResourcesFolderPath + "Shaders/Vulkan/DebugMesh.frag";
			};

			pipelineCI.PipelineName = "DebugMesh";
			pipelineCI.eCullMode = CullMode::None;
			pipelineCI.VertexInputInfos = { VertexInputInfo(sizeof(PBRVertex), layout) };
			pipelineCI.ePolygonMode = PolygonMode::Line;
			pipelineCI.bDepthTestEnabled = false;
			pipelineCI.pTargetFramebuffer = GraphicsContext::GetSingleton()->GetFramebuffer();
			pipelineCI.pShaderCreateInfo = &shaderCI;

			assert(s_Data->MeshPipeline.Create(&pipelineCI) == PipelineCreateResult::SUCCESS);
		}

	}
}