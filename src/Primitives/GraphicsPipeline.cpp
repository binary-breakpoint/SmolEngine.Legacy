#include "stdafx.h"
#include "Primitives/GraphicsPipeline.h"
#include "Primitives/Framebuffer.h"
#include "Primitives/Shader.h"
#include "Primitives/Mesh.h"

#ifndef OPENGL_IMPL
#include "Backends/Vulkan/VulkanPipeline.h"
#endif

namespace SmolEngine
{
	GraphicsPipeline::~GraphicsPipeline()
	{
		Destroy();
	}

	Ref<GraphicsPipeline> GraphicsPipeline::Create()
	{
		Ref<GraphicsPipeline> pipeline = nullptr;
#ifdef OPENGL_IMPL
#else
		pipeline = std::make_shared<VulkanPipeline>();
#endif 
		return pipeline;
	}

	void GraphicsPipeline::Reset()
	{
		m_DescriptorIndex = 0;
		m_DrawMode = DrawMode::Triangle;
	}

	void GraphicsPipeline::SetFramebuffers(const std::vector<Framebuffer*>& fb)
	{
		m_PiplineCreateInfo.TargetFramebuffers = fb;
	}

	void GraphicsPipeline::SetVertexBuffers(const std::vector<Ref<VertexBuffer>>& vb)
	{
		m_VertexBuffers = vb;
	}

	void GraphicsPipeline::SetIndexBuffers(const std::vector<Ref<IndexBuffer>>& ib)
	{
		m_IndexBuffers = ib;
	}

	void GraphicsPipeline::SetDescriptorIndex(uint32_t value)
	{
		m_DescriptorIndex = value;
	}

	void GraphicsPipeline::SetDrawMode(DrawMode mode)
	{
		m_DrawMode = mode;
	}

	void GraphicsPipeline::SetFramebufferIndex(uint32_t index)
	{
		m_FBIndex = index;
	}

	void GraphicsPipeline::SetFramebufferAttachmentIndex(uint32_t index)
	{
		m_FBattachmentIndex = index;
	}

	bool GraphicsPipeline::InvalidateBase(GraphicsPipelineCreateInfo* pipelineInfo)
	{
		if (!IsPipelineCreateInfoValid(pipelineInfo))
			return false;

		m_GraphicsContext = GraphicsContext::s_Instance;
		m_Shader = std::make_shared<Shader>();
		m_PiplineCreateInfo = *pipelineInfo;
		Shader::Create(&m_PiplineCreateInfo.ShaderCreateInfo, m_Shader.get());
		return true;
	}

	bool GraphicsPipeline::IsPipelineCreateInfoValid(const GraphicsPipelineCreateInfo* pipelineInfo)
	{
		if (pipelineInfo->DescriptorSets < 1 || pipelineInfo->TargetFramebuffers.size() == 0)
			return false;

		return true;
	}
}