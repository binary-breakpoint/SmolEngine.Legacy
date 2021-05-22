#include "stdafx.h"
#include "DebugRenderer.h"

#include "Common/Framebuffer.h"
#include "Common/Mesh.h"
#include "Utils/Utils.h"
#include "GraphicsPipeline.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{

#define SIMD_PI float(3.1415926535897932384626433832795029)
#define SIMD_HALF_PI (SIMD_PI * 0.5f)
#define SIMD_2_PI (2.0f * SIMD_PI)
#define SIMD_RADS_PER_DEG (SIMD_2_PI / float(360.0))

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
		glm::vec4          Color = glm::vec4(0, 1, 0, 1);
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

	void DebugRenderer::DrawSphereLines(const glm::vec3& center, const glm::vec3& up, const glm::vec3& axis, float radius, float minTh, float maxTh, float minPs, float maxPs, float stepDegrees, bool drawCenter)
	{
		glm::vec3 vA[74];
		glm::vec3 vB[74];
		glm::vec3* pvA = vA, * pvB = vB, * pT;
		glm::vec3 npole = center + up * radius;
		glm::vec3 spole = center - up * radius;
		glm::vec3 arcStart;
		float step = stepDegrees * SIMD_RADS_PER_DEG;
		const glm::vec3& kv = up;
		const glm::vec3& iv = axis;

		glm::vec3 jv = glm::vec3(
			kv[1] * iv[2] - kv[2] *iv[1],
			kv[2] * iv[0] - kv[0] *iv[2],
			kv[0] * iv[1] - kv[1] *iv[0]);

		bool drawN = false;
		bool drawS = false;
		if (minTh <= -SIMD_HALF_PI)
		{
			minTh = -SIMD_HALF_PI + step;
			drawN = true;
		}
		if (maxTh >= SIMD_HALF_PI)
		{
			maxTh = SIMD_HALF_PI - step;
			drawS = true;
		}
		if (minTh > maxTh)
		{
			minTh = -SIMD_HALF_PI + step;
			maxTh = SIMD_HALF_PI - step;
			drawN = drawS = true;
		}
		int n_hor = (int)((maxTh - minTh) / step) + 1;
		if (n_hor < 2) n_hor = 2;
		float step_h = (maxTh - minTh) /float(n_hor - 1);
		bool isClosed = false;
		if (minPs > maxPs)
		{
			minPs = -SIMD_PI + step;
			maxPs = SIMD_PI;
			isClosed = true;
		}
		else if ((maxPs - minPs) >= SIMD_PI * float(2.f))
		{
			isClosed = true;
		}
		else
		{
			isClosed = false;
		}
		int n_vert = (int)((maxPs - minPs) / step) + 1;
		if (n_vert < 2) n_vert = 2;
		float step_v = (maxPs - minPs) /float(n_vert - 1);
		for (int i = 0; i < n_hor; i++)
		{
			float th = minTh + float(i) * step_h;
			float sth = radius * sinf(th);
			float cth = radius * cosf(th);
			for (int j = 0; j < n_vert; j++)
			{
				float psi = minPs + float(j) * step_v;
				float sps = sinf(psi);
				float cps = cosf(psi);
				pvB[j] = center + cth * cps * iv + cth * sps * jv + sth * kv;
				if (i)
				{
					DrawLine(pvA[j], pvB[j]);
				}
				else if (drawS)
				{
					DrawLine(spole, pvB[j]);
				}
				if (j)
				{
					DrawLine(pvB[j - 1], pvB[j]);
				}
				else
				{
					arcStart = pvB[j];
				}
				if ((i == (n_hor - 1)) && drawN)
				{
					DrawLine(npole, pvB[j]);
				}

				if (drawCenter)
				{
					if (isClosed)
					{
						if (j == (n_vert - 1))
						{
							DrawLine(arcStart, pvB[j]);
						}
					}
					else
					{
						if (((!i) || (i == (n_hor - 1))) && ((!j) || (j == (n_vert - 1))))
						{
							DrawLine(center, pvB[j]);
						}
					}
				}
			}
			pT = pvA;
			pvA = pvB;
			pvB = pT;
		}
	}

	void DebugRenderer::DrawSphere(float radius, const glm::vec3& pos, const glm::vec3& rot, const glm::vec3& scale)
	{
		glm::mat4 model;
		Utils::ComposeTransform(pos, rot, scale, model);

		glm::vec3 up = model[1];
		glm::vec3 axis = model[0];
		float minTh = -SIMD_HALF_PI;
		float maxTh = SIMD_HALF_PI;
		float minPs = -SIMD_HALF_PI;
		float maxPs = SIMD_HALF_PI;
		float stepDegrees = 30.f;
		DrawSphereLines(pos, up, axis, radius, minTh, maxTh, minPs, maxPs, stepDegrees, false);
		DrawSphereLines(pos, up, -axis, radius, minTh, maxTh, minPs, maxPs, stepDegrees, false);
	}

	void DebugRenderer::DrawCapsule(float radius, float halfHeight, uint32_t upAxis, const glm::vec3& pos, const glm::vec3& rot_, const glm::vec3& scale)
	{
		int stepDegrees = 30;
		glm::vec3 capStart(0.f, 0.f, 0.f);
		capStart[upAxis] = -halfHeight;

		glm::vec3 capEnd(0.f, 0.f, 0.f);
		capEnd[upAxis] = halfHeight;

		glm::mat4 transform;
		Utils::ComposeTransform(pos, rot_, scale, transform);

		// Draw the ends
		{
			glm::mat4 childTransform = transform;
			glm::vec4 origin = transform * glm::vec4(capStart, 1.0);
			{
				glm::vec3 center = origin;
				glm::vec3 up = transform[(upAxis + 1) % 3];
				glm::vec3 axis = -transform[upAxis];
				float minTh = -SIMD_HALF_PI;
				float maxTh = SIMD_HALF_PI;
				float minPs = -SIMD_HALF_PI;
				float maxPs = SIMD_HALF_PI;

				DrawSphereLines(center, up, axis, radius, minTh, maxTh, minPs, maxPs, static_cast<float>(stepDegrees), false);
			}
		}

		{
			glm::mat4 childTransform = transform;
			glm::vec4 origin = transform * glm::vec4(capStart, 1.0);
			{
				glm::vec3 center = origin;
				glm::vec3 up = transform[(upAxis + 1) % 3];
				glm::vec3 axis = transform[upAxis];
				float minTh = -SIMD_HALF_PI;
				float maxTh = SIMD_HALF_PI;
				float minPs = -SIMD_HALF_PI;
				float maxPs = SIMD_HALF_PI;
				DrawSphereLines(center, up, axis, radius, minTh, maxTh, minPs, maxPs, float(stepDegrees), false);
			}
		}
	}

	void DebugRenderer::DrawBox(const glm::vec3& bbMin, const glm::vec3& bbMax, const glm::vec3& pos, const glm::vec3& rot, const glm::vec3& scale)
	{
		glm::mat4 trans;
		Utils::ComposeTransform(pos, rot, scale, trans);

		DrawLine(trans * glm::vec4(bbMin[0], bbMin[1], bbMin[2], 1), trans * glm::vec4(bbMax[0], bbMin[1], bbMin[2], 1));
		DrawLine(trans * glm::vec4(bbMax[0], bbMin[1], bbMin[2], 1), trans * glm::vec4(bbMax[0], bbMax[1], bbMin[2], 1));
		DrawLine(trans * glm::vec4(bbMax[0], bbMax[1], bbMin[2], 1), trans * glm::vec4(bbMin[0], bbMax[1], bbMin[2], 1));
		DrawLine(trans * glm::vec4(bbMin[0], bbMax[1], bbMin[2], 1), trans * glm::vec4(bbMin[0], bbMin[1], bbMin[2], 1));
		DrawLine(trans * glm::vec4(bbMin[0], bbMin[1], bbMin[2], 1), trans * glm::vec4(bbMin[0], bbMin[1], bbMax[2], 1));
		DrawLine(trans * glm::vec4(bbMax[0], bbMin[1], bbMin[2], 1), trans * glm::vec4(bbMax[0], bbMin[1], bbMax[2], 1));
		DrawLine(trans * glm::vec4(bbMax[0], bbMax[1], bbMin[2], 1), trans * glm::vec4(bbMax[0], bbMax[1], bbMax[2], 1));
		DrawLine(trans * glm::vec4(bbMin[0], bbMax[1], bbMin[2], 1), trans * glm::vec4(bbMin[0], bbMax[1], bbMax[2], 1));
		DrawLine(trans * glm::vec4(bbMin[0], bbMin[1], bbMax[2], 1), trans * glm::vec4(bbMax[0], bbMin[1], bbMax[2], 1));
		DrawLine(trans * glm::vec4(bbMax[0], bbMin[1], bbMax[2], 1), trans * glm::vec4(bbMax[0], bbMax[1], bbMax[2], 1));
		DrawLine(trans * glm::vec4(bbMax[0], bbMax[1], bbMax[2], 1), trans * glm::vec4(bbMin[0], bbMax[1], bbMax[2], 1));
		DrawLine(trans * glm::vec4(bbMin[0], bbMax[1], bbMax[2], 1), trans * glm::vec4(bbMin[0], bbMin[1], bbMax[2], 1));
	}

	void DebugRenderer::DrawAABB(const BoundingBox& aabb, const glm::vec3& pos, const glm::vec3& scale)
	{
		glm::mat4 scaleMat = glm::scale(scale);
		glm::vec3 halfExtents = (aabb.max - aabb.min) * 0.5f;
		int i, j;

		glm::vec3 edgecoord(1.f, 1.f, 1.f), pa, pb;
		for (i = 0; i < 4; i++)
		{
			for (j = 0; j < 3; j++)
			{
				pa = glm::vec3(edgecoord[0] * halfExtents[0], edgecoord[1] * halfExtents[1],
					edgecoord[2] * halfExtents[2]);
				pa = scaleMat * glm::vec4(pa, 1.0);
				pa += pos;

				int othercoord = j % 3;
				edgecoord[othercoord] *= -1.f;
				pb = glm::vec3(edgecoord[0] * halfExtents[0], edgecoord[1] * halfExtents[1],
					edgecoord[2] * halfExtents[2]);

				pb = scaleMat * glm::vec4(pb, 1.0);
				pb += pos;

				DrawLine(pa, pb);
			}

			edgecoord = glm::vec3(-1.f, -1.f, -1.f);
			if (i < 3)
				edgecoord[i] *= -1.f;
		}

	}

	void DebugRenderer::DrawLine(const glm::vec3& pos1, const glm::vec3& pos2, float width)
	{
		s_Data->PushConst.State = 1;
		Utils::ComposeTransform(pos1, { 0, 0, 0 }, { 1, 1, 1}, s_Data->PushConst.Model);
		Utils::ComposeTransform(pos2, { 0, 0, 0 }, { 1, 1, 1 }, s_Data->PushConst.Model_2);

		s_Data->PrimitivePipeline.SubmitPushConstant(ShaderType::Vertex, sizeof(PushConstant), &s_Data->PushConst);
		s_Data->PrimitivePipeline.SetDrawMode(DrawMode::Line);
		s_Data->PrimitivePipeline.Draw(&s_Data->LineVB, 2);
		s_Data->PrimitivePipeline.ResetStates();
	}

	void DebugRenderer::DrawQuad(const glm::vec3& pos, const glm::vec3& rotation, const glm::vec3& scale)
	{
		s_Data->PushConst.State = 0;
		Utils::ComposeTransform(pos, rotation, scale, s_Data->PushConst.Model);
		s_Data->PrimitivePipeline.SubmitPushConstant(ShaderType::Vertex, sizeof(PushConstant), &s_Data->PushConst);
		s_Data->PrimitivePipeline.SetDrawMode(DrawMode::Line);
		s_Data->PrimitivePipeline.DrawIndexed(&s_Data->QuadVB, &s_Data->QuadIB);
		s_Data->PrimitivePipeline.ResetStates();
	}

	void DebugRenderer::DrawCirlce(const glm::vec3& pos, const glm::vec3& scale)
	{
		s_Data->PushConst.State = 0;
		Utils::ComposeTransform(pos, { 0,0,0 }, scale, s_Data->PushConst.Model);
		s_Data->PrimitivePipeline.SubmitPushConstant(ShaderType::Vertex, sizeof(PushConstant), &s_Data->PushConst);
		s_Data->PrimitivePipeline.SetDrawMode(DrawMode::Fan);
		s_Data->PrimitivePipeline.Draw(&s_Data->CircleVB, 3000);
		s_Data->PrimitivePipeline.ResetStates();
	}

	void DebugRenderer::SetColor(const glm::vec4& color)
	{
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
			pipelineCI.ShaderCreateInfo = shaderCI;

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
			pipelineCI.ShaderCreateInfo = shaderCI;

			assert(s_Data->WireframesPipeline.Create(&pipelineCI) == PipelineCreateResult::SUCCESS);
		}

	}
}