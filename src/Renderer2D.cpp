#include "stdafx.h"
#include "Renderer2D.h"
#include "GraphicsPipeline.h"

#include "Utils/Utils.h"

namespace Frostium
{
	static const uint32_t s_SamplersBindingPoint = 2;

	struct Renderer2DStorage
	{
		static const uint32_t Light2DBufferMaxSize = 100;
		static const uint32_t MaxQuads = 15000;
		static const uint32_t MaxVertices = MaxQuads * 6;
		static const uint32_t MaxIndices = MaxQuads * 4;
		static const uint32_t MaxLayers = 12;

		/// Graphics Pipelines

		Ref<GraphicsPipeline> MainPipeline = nullptr;

		/// Light

		Light2DBuffer LightBuffer[Light2DBufferMaxSize];

	};

	Renderer2DStats* Renderer2D::Stats = new Renderer2DStats();
	static Renderer2DStorage* s_Data;

	void Renderer2D::Init()
	{
		return;
		s_Data = new Renderer2DStorage();

		CreateBatchData();
	}

	void Renderer2D::BeginScene(const ClearInfo* clearInfo, const BeginSceneInfo* info)
	{
		s_Data->MainPipeline->BeginCommandBuffer();
		s_Data->MainPipeline->BeginRenderPass();
		{
			s_Data->MainPipeline->ClearColors();
		}
		s_Data->MainPipeline->EndRenderPass();
		s_Data->MainPipeline->BeginBufferSubmit();

		StartNewBatch();
		Stats->Reset();
	}

	void Renderer2D::EndScene()
	{
		return; //

#ifndef FROSTIUM_OPENGL_IMPL
		s_Data->MainPipeline->EndBufferSubmit();
		s_Data->MainPipeline->EndCommandBuffer();
#endif
	}

	void Renderer2D::StartNewBatch()
	{

	}

	void Renderer2D::UploadLightUniforms()
	{
	}

	void Renderer2D::ClearBuffers()
	{

	}

	void Renderer2D::DrawSprite(const glm::vec3& worldPos, const uint32_t layerIndex, const glm::vec2& scale, const float rotation, const Ref<Texture>& texture)
	{
	}

	void Renderer2D::DrawQuad(const glm::vec3& worldPos, const uint32_t layerIndex, const glm::vec2& scale, const float rotation, const glm::vec4& color)
	{
	}

	void Renderer2D::Shutdown()
	{
		delete s_Data;
	}

	void Renderer2D::SubmitLight2D(const glm::vec3& offset, const float radius, const glm::vec4& color, const float lightIntensity)
	{

	}

	void Renderer2D::DrawFrameBuffer(const uint32_t colorAttachmentID)
	{

	}

	bool Renderer2D::FindShader(std::string& filePath, const std::string& shaderName)
	{
		using recursive_directory_iterator = std::filesystem::recursive_directory_iterator;

		for (const auto& dirEntry : recursive_directory_iterator(std::string("C:/Dev/Frostium/")))
		{
			if (dirEntry.path().filename() == shaderName)
			{
				filePath = dirEntry.path().string();
				return true;
			}
		}

		return false;
	}

	void Renderer2D::CreateBatchData()
	{


		// Creating and uploading indices
		s_Data->MainPipeline = std::make_shared<GraphicsPipeline>();

		uint32_t* quadIndices = new uint32_t[s_Data->MaxIndices];
		{
			uint32_t offset = 0;
			for (uint32_t i = 0; i < s_Data->MaxIndices; i += 6)
			{
				quadIndices[i + 0] = offset + 0;
				quadIndices[i + 1] = offset + 1;
				quadIndices[i + 2] = offset + 2;

				quadIndices[i + 3] = offset + 2;
				quadIndices[i + 4] = offset + 3;
				quadIndices[i + 5] = offset + 0;

				offset += 4;
			}
		}

		BufferLayout layout(
		{
			{ DataTypes::Float3, "a_Position" }, // layout(location = 0)
			{ DataTypes::Float4, "a_Color"},
			{ DataTypes::Float2, "a_TexCoord" },
			{ DataTypes::Float, "a_TextMode"},
			{ DataTypes::Float, "a_TextureIndex"} // layout(location = 4)
		});

		//VertexBuffers
		std::vector<Ref<VertexBuffer>> vertexBuffers;
		uint32_t vertexBufferCount = 1;
#ifndef FROSTIUM_OPENGL_IMPL
		vertexBufferCount = s_Data->MaxLayers;
#endif
		vertexBuffers.resize(vertexBufferCount);
		for (uint32_t i = 0; i < vertexBufferCount; ++i)
		{
			vertexBuffers[i] = VertexBuffer::Create(sizeof(QuadVertex) * s_Data->MaxVertices);
		}

		//IndexBuffers
		Ref<IndexBuffer> indexBuffer = IndexBuffer::Create(quadIndices, s_Data->MaxIndices);

		GraphicsPipelineShaderCreateInfo shaderCI = {};
		{
#ifdef FROSTIUM_OPENGL_IMPL
			shaderCI.UseSingleFile = true;
			shaderCI.SingleFilePath = "../Resources/Shaders/BaseShader2D_OpenGL.glsl";
#else
			shaderCI.FilePaths[ShaderType::Vertex] = "../Resources/Shaders/BaseShader2D_Vulkan_Vertex.glsl";
			shaderCI.FilePaths[ShaderType::Fragment] = "../Resources/Shaders/BaseShader2D_Vulkan_Fragment.glsl";
#endif
		}

		GraphicsPipelineCreateInfo graphicsPipelineCI = {};
		{
			graphicsPipelineCI.VertexInputInfos = { VertexInputInfo(sizeof(QuadVertex), layout, false) };
			graphicsPipelineCI.ShaderCreateInfo = &shaderCI;

			graphicsPipelineCI.DescriptorSets = Renderer2DStorage::MaxLayers;
			graphicsPipelineCI.PipelineName = "Renderer2D_Main";
		}

		s_Data->MainPipeline->Create(&graphicsPipelineCI);

		//
		s_Data->MainPipeline->SetVertexBuffers(vertexBuffers);
		s_Data->MainPipeline->SetIndexBuffers({ indexBuffer });

		delete[] quadIndices;

#ifdef FROSTIUM_OPENGL_IMPL
		// Loading samplers
		int32_t samplers[MaxTextureSlot];
		for (uint32_t i = 0; i < MaxTextureSlot; ++i)
		{
			samplers[i] = i;
		}

		s_Data->MainPipeline->SumbitUniform<int*>("u_Textures", samplers, MaxTextureSlot);
		for (uint32_t i = 0; i < MaxTextureSlot; ++i)
		{
			s_Data->WhiteTexture->Bind(i);
		}


#endif // FROSTIUM_OPENGL_IMPL
	}

	void Renderer2D::CreateFramebufferData()
	{
#ifdef Frostium_EDITOR


#endif
	}

}