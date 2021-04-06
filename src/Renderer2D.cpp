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

	void Renderer2D::SubmitSprite(const glm::vec3& worldPos, const uint32_t layerIndex, const glm::vec2& scale, const float rotation, const Ref<Texture>& texture)
	{
	}

	void Renderer2D::SubmitQuad(const glm::vec3& worldPos, const uint32_t layerIndex, const glm::vec2& scale, const float rotation, const glm::vec4& color)
	{
	}

	void Renderer2D::CreatePipelines()
	{
	}

	void Renderer2D::CreateFramebuffers()
	{
	}

	void Renderer2D::StartNewBatch()
	{

	}

	void Renderer2D::UploadLightUniforms()
	{

	}

	void Renderer2D::Shutdown()
	{
		delete s_Data;
	}

	void Renderer2D::SubmitText(const glm::vec3& pos, const glm::vec2& scale, const Ref<Texture> texture, const glm::vec4& tintColor)
	{

	}

	void Renderer2D::SubmitLight2D(const glm::vec3& offset, const float radius, const glm::vec4& color, const float lightIntensity)
	{

	}
}