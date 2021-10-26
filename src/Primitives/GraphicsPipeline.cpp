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
		m_FBIndex = 0;
		m_FBattachmentIndex = 0;
		m_DrawMode = DrawMode::Triangle;
	}

	void GraphicsPipeline::SetFramebuffers(const std::vector<Ref<Framebuffer>>& fb)
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

	bool GraphicsPipeline::BuildBase(GraphicsPipelineCreateInfo* pipelineInfo)
	{
		if (!IsPipelineCreateInfoValid(pipelineInfo))
			return false;

		m_GraphicsContext = GraphicsContext::s_Instance;
		m_PiplineCreateInfo = *pipelineInfo;

		m_Shader = Shader::Create();
		m_Shader->Build(&m_PiplineCreateInfo.ShaderCreateInfo);
		return true;
	}

	bool GraphicsPipeline::IsPipelineCreateInfoValid(const GraphicsPipelineCreateInfo* pipelineInfo)
	{
		if (pipelineInfo->DescriptorSets < 1 || pipelineInfo->TargetFramebuffers.size() == 0)
			return false;

		return true;
	}
}