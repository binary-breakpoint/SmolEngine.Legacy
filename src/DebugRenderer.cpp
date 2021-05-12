#include "stdafx.h"
#include "DebugRenderer.h"

#include "Common/Framebuffer.h"
#include "Common/Mesh.h"
#include "Utils/Utils.h"
#include "GraphicsPipeline.h"

namespace Frostium
{
	struct DebugVertex
	{
		glm::vec3          Position;
	};

	struct PushConstant
	{
		glm::mat4          Model = glm::mat4(1.0f);
		glm::mat4          Model_2 = glm::mat4(1.0f);;
		uint32_t           State = 0;
	};

	struct DebugRendererStorage
	{
		GraphicsPipeline   PrimitivePipeline{};
		GraphicsPipeline   WireframesPipeline{};	   
		VertexBuffer       QuadVB{};
		VertexBuffer       CircleVB{};
		VertexBuffer       LineVB{};
		IndexBuffer        QuadIB{};
		PushConstant       PushConst{};
	};					   

	DebugRendererStorage* s_Data = nullptr;

	void DebugRenderer::BeginDebug()
	{
		s_Data->WireframesPipeline.BeginCommandBuffer(true);
		s_Data->PrimitivePipeline.BeginCommandBuffer(true);
		s_Data->WireframesPipeline.BeginRenderPass();
	}

	void DebugRenderer::EndDebug()
	{
		s_Data->WireframesPipeline.EndRenderPass();
	}

	void DebugRenderer::DrawLine(const glm::vec3& pos1, const glm::vec3& pos2, float width, const glm::vec4& color)
	{
		s_Data->PushConst.State = 1;
		Utils::ComposeTransform(pos1, { 0, 0, 0 }, { 1, 1, 1}, s_Data->PushConst.Model);
		Utils::ComposeTransform(pos2, { 0, 0, 0 }, { 1, 1, 1 }, s_Data->PushConst.Model_2);

		s_Data->PrimitivePipeline.SubmitPushConstant(ShaderType::Vertex, sizeof(PushConstant), &s_Data->PushConst);
		s_Data->PrimitivePipeline.Draw(&s_Data->LineVB, 2, DrawMode::Line);
	}

	void DebugRenderer::DrawQuad(const glm::vec3& pos, const glm::vec3& rotation, const glm::vec3& scale)
	{
		s_Data->PushConst.State = 0;
		Utils::ComposeTransform(pos, rotation, scale, s_Data->PushConst.Model);
		s_Data->PrimitivePipeline.SubmitPushConstant(ShaderType::Vertex, sizeof(PushConstant), &s_Data->PushConst);
		s_Data->PrimitivePipeline.DrawIndexed(&s_Data->QuadVB, &s_Data->QuadIB, DrawMode::Line);
	}

	void DebugRenderer::DrawCirlce(const glm::vec3& pos, const glm::vec3& scale)
	{
		s_Data->PushConst.State = 0;
		Utils::ComposeTransform(pos, { 0,0,0 }, scale, s_Data->PushConst.Model);
		s_Data->PrimitivePipeline.SubmitPushConstant(ShaderType::Vertex, sizeof(PushConstant), &s_Data->PushConst);
		s_Data->PrimitivePipeline.Draw(&s_Data->CircleVB, 3000, DrawMode::Fan);
	}

	void DebugRenderer::DrawWireframes(const glm::vec3& pos, const glm::vec3& rotation, const glm::vec3& scale, Mesh* mesh)
	{
		glm::mat4 model;
		Utils::ComposeTransform(pos, rotation, scale, model);
		s_Data->WireframesPipeline.SubmitPushConstant(ShaderType::Vertex, sizeof(glm::mat4), &model);
		s_Data->WireframesPipeline.DrawMeshIndexed(mesh, 1);

		for(auto& child: mesh->GetChilds())
			DrawWireframes(pos, rotation, scale, &child);
	}

	void DebugRenderer::Init()
	{
		s_Data = new DebugRendererStorage();

		VertexInputInfo vertexInput{};
		{
			BufferLayout main_layout =
			{
				{ DataTypes::Float3, "aPos" },
				{ DataTypes::Float3, "aNormal" },
				{ DataTypes::Float3, "aTangent" },
				{ DataTypes::Float2, "aUV" },
				{ DataTypes::Int4,   "aBoneIDs"},
				{ DataTypes::Float4, "aWeight"}
			};

			vertexInput = VertexInputInfo(sizeof(glm::vec3), main_layout);
		}

		// Primitives
		{
			{
				// Quad
				DebugVertex QuadVertex[4];
				QuadVertex[0].Position = { -0.5f, -0.5f, 0.0f };
				QuadVertex[1].Position = { 0.5f, -0.5f, 0.0f };
				QuadVertex[2].Position = { 0.5f,  0.5f, 0.0f };
				QuadVertex[3].Position = { -0.5f,  0.5f, 0.0f };
				uint32_t quadIndices[6] = { 0, 1, 2,  2, 3, 0 };

				// Cirlce
				const size_t nVertices = 3000;
				DebugVertex* CircleVertex = new DebugVertex[nVertices];
				for (size_t i = 1; i < nVertices; i++)
				{
					CircleVertex[i].Position = glm::vec3(cos(2 * 3.14159 * i / 1000.0), sin(2 * 3.14159 * i / 1000.0), 0);
				}

				// Line
				DebugVertex LineVertex[2];
				LineVertex[0].Position = { 0.0f, 0.0f, 0.0f };
				LineVertex[1].Position = { 0.0f, 0.0f, 0.0f };

				bool isStatic = true;
				VertexBuffer::Create(&s_Data->QuadVB, &QuadVertex, sizeof(DebugVertex) * 4, isStatic);
				VertexBuffer::Create(&s_Data->CircleVB, CircleVertex, sizeof(DebugVertex) * nVertices, isStatic);
				VertexBuffer::Create(&s_Data->LineVB, &LineVertex, sizeof(DebugVertex) * 2, isStatic);
				IndexBuffer::Create(&s_Data->QuadIB, quadIndices, 6, isStatic);

				delete[] CircleVertex;
			}

			BufferLayout layout =
			{
				{ DataTypes::Float3, "aPos" }
			};

			GraphicsPipelineCreateInfo pipelineCI = {};
			GraphicsPipelineShaderCreateInfo shaderCI = {};
			{
				shaderCI.FilePaths[ShaderType::Vertex] = GraphicsContext::GetSingleton()->m_ResourcesFolderPath + "Shaders/DebugPrimitive.vert";
				shaderCI.FilePaths[ShaderType::Fragment] = GraphicsContext::GetSingleton()->m_ResourcesFolderPath + "Shaders/DebugColor.frag";
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

		// Wireframes
		{

			GraphicsPipelineCreateInfo pipelineCI = {};
			GraphicsPipelineShaderCreateInfo shaderCI = {};
			{
				shaderCI.FilePaths[ShaderType::Vertex] = GraphicsContext::GetSingleton()->m_ResourcesFolderPath + "Shaders/DebugMesh.vert";
				shaderCI.FilePaths[ShaderType::Fragment] = GraphicsContext::GetSingleton()->m_ResourcesFolderPath + "Shaders/DebugColor.frag";
			};

			pipelineCI.PipelineName = "DebugMesh";
			pipelineCI.eCullMode = CullMode::None;
			pipelineCI.VertexInputInfos = { vertexInput };
			pipelineCI.ePolygonMode = PolygonMode::Line;
			pipelineCI.bDepthTestEnabled = false;
			pipelineCI.pTargetFramebuffer = GraphicsContext::GetSingleton()->GetFramebuffer();
			pipelineCI.pShaderCreateInfo = &shaderCI;

			assert(s_Data->WireframesPipeline.Create(&pipelineCI) == PipelineCreateResult::SUCCESS);
		}

	}
}