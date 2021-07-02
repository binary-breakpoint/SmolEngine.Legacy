#include "stdafx.h"
#include "GraphicsPipeline.h"

#ifndef FROSTIUM_OPENGL_IMPL
#include "Vulkan/VulkanContext.h"
#include "Vulkan/VulkanRenderPass.h"
#include "Vulkan/VulkanTexture.h"
#include "Vulkan/VulkanBufferPool.h"
#endif

#include "Primitives/Framebuffer.h"
#include "Primitives/CubeMap.h"
#include "Primitives/Shader.h"
#include "Primitives/Mesh.h"

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	GraphicsPipeline::~GraphicsPipeline()
	{
		Destroy();
	}

	PipelineCreateResult GraphicsPipeline::Create(GraphicsPipelineCreateInfo* pipelineInfo)
	{
		if(!IsPipelineCreateInfoValid(pipelineInfo))
			return PipelineCreateResult::ERROR_INVALID_CREATE_INFO;

		m_GraphicsContext = GraphicsContext::s_Instance;
		m_Shader = std::make_shared<Shader>();
		m_PiplineCreateInfo = *pipelineInfo;
		Shader::Create(&m_PiplineCreateInfo.ShaderCreateInfo, m_Shader.get());

#ifdef FROSTIUM_OPENGL_IMPL
		m_VextexArray = VertexArray::Create();
#else
		m_VulkanPipeline = {};
		if (!m_VulkanPipeline.Invalidate(&m_PiplineCreateInfo, m_Shader->GetVulkanShader()))
			return PipelineCreateResult::ERROR_PIPELINE_NOT_INVALIDATED;

		for (DrawMode mode : pipelineInfo->PipelineDrawModes)
		{
			if (!m_VulkanPipeline.CreatePipeline(mode))
			{
				m_Shader->GetVulkanShader()->DeleteShaderModules();
				return PipelineCreateResult::ERROR_PIPELINE_NOT_CREATED;
			}
		}
		m_Shader->GetVulkanShader()->DeleteShaderModules();
#endif
		m_Shader->m_ReflectData.Clean();
		return PipelineCreateResult::SUCCESS;
	}

	PipelineCreateResult GraphicsPipeline::Reload()
	{
		if (!m_Shader->Realod())
			return PipelineCreateResult::ERROR_SHADER_NOT_RELOADED;

#ifndef FROSTIUM_OPENGL_IMPL
		if (!m_VulkanPipeline.ReCreate())
			return PipelineCreateResult::ERROR_PIPELINE_NOT_INVALIDATED;
		m_Shader->GetVulkanShader()->DeleteShaderModules();
#endif
		m_Shader->m_ReflectData.Clean();
		return PipelineCreateResult::SUCCESS;
	}

	void GraphicsPipeline::Destroy()
	{
		m_VertexBuffers.clear();
		m_IndexBuffers.clear();

#ifndef FROSTIUM_OPENGL_IMPL
		m_VulkanPipeline.Destroy();
		m_VulkanPipeline = {};
#endif
		if (m_Shader)
			m_Shader = nullptr;

		if (m_VextexArray)
			m_VextexArray = nullptr;
	}

	void GraphicsPipeline::BeginRenderPass(bool flip)
	{
		Framebuffer* target_fb = m_PiplineCreateInfo.TargetFramebuffers[m_FBIndex];
#ifdef FROSTIUM_OPENGL_IMPL
		m_Shader->Bind();
		m_VextexArray->Bind();
		target_fb->Bind();
#else
		auto& vulkanFB = target_fb->GetVulkanFramebuffer();
		const VkFramebuffer vkFramebuffer =  m_FBattachmentIndex == 0 ? vulkanFB.GetCurrentVkFramebuffer(): vulkanFB.GetVkFramebuffer(m_FBattachmentIndex);
		const FramebufferSpecification& fb_specs = target_fb->GetSpecification();
		uint32_t width = fb_specs.Width;
		uint32_t height = fb_specs.Height;

		VkRenderPassBeginInfo renderPassBeginInfo = {};
		{
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.renderPass = vulkanFB.GetRenderPass();
			renderPassBeginInfo.framebuffer = vkFramebuffer;
			renderPassBeginInfo.renderArea.offset.x = 0;
			renderPassBeginInfo.renderArea.offset.y = 0;
			renderPassBeginInfo.renderArea.extent.width = width;
			renderPassBeginInfo.renderArea.extent.height = height;
			renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(vulkanFB.GetClearValues().size());
			renderPassBeginInfo.pClearValues = vulkanFB.GetClearValues().data();
		}

		vkCmdBeginRenderPass(m_CommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		// Update dynamic viewport state
		VkViewport viewport = {};
		if (flip)
		{
			viewport.height = (float)height;
			viewport.width = (float)width;
			viewport.minDepth = (float)0.0f;
			viewport.maxDepth = (float)1.0f;
		}
		else
		{
			viewport.x = 0;
			viewport.y = (float)height;
			viewport.height = -(float)height;
			viewport.width = (float)width;
			viewport.minDepth = (float)0.0f;
			viewport.maxDepth = (float)1.0f;
		}

		vkCmdSetViewport(m_CommandBuffer, 0, 1, &viewport);

		// Update dynamic scissor state
		VkRect2D scissor = {};
		scissor.extent.width = width;
		scissor.extent.height = height;
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		vkCmdSetScissor(m_CommandBuffer, 0, 1, &scissor);

#endif // FROSTIUM_OPENGL_IMPL
	}

	void GraphicsPipeline::EndRenderPass()
	{
#ifdef FROSTIUM_OPENGL_IMPL
		m_PiplineCreateInfo.TargetFramebuffer->UnBind();
		m_Shader->UnBind();
		m_VextexArray->UnBind();
#else
		vkCmdEndRenderPass(m_CommandBuffer);
#endif
	}

	void GraphicsPipeline::ClearColors(const glm::vec4& clearColors)
	{
#ifdef FROSTIUM_OPENGL_IMPL
		OpenglRendererAPI* instance = m_GraphicsContext->GetOpenglRendererAPI();
		instance->SetClearColor(clearColors);
		instance->Clear();
#else
		Framebuffer* target_fb = m_PiplineCreateInfo.TargetFramebuffers[m_FBIndex];
		VkClearRect clearRect = {};

		clearRect.layerCount = 1;
		clearRect.baseArrayLayer = 0;
		clearRect.rect.offset = { 0, 0 };
		clearRect.rect.extent = { (uint32_t)target_fb->GetSpecification().Width, (uint32_t)target_fb->GetSpecification().Height };

		auto& framebuffer = target_fb->GetVulkanFramebuffer();
		framebuffer.SetClearColors(clearColors);

		vkCmdClearAttachments(m_CommandBuffer, static_cast<uint32_t>(framebuffer.GetClearAttachments().size()),
			framebuffer.GetClearAttachments().data(), 1, &clearRect);
#endif
	}

	bool GraphicsPipeline::SubmitBuffer(uint32_t bindingPoint, size_t size, const void* data, uint32_t offset)
	{
#ifndef FROSTIUM_OPENGL_IMPL
		return m_VulkanPipeline.m_Descriptors[m_DescriptorIndex].UpdateBuffer(bindingPoint, size, data, offset);
#else
		return false;
#endif
	}

	void GraphicsPipeline::BeginCommandBuffer(bool isMainCmdBufferInUse)
	{
#ifndef FROSTIUM_OPENGL_IMPL
		if (isMainCmdBufferInUse)
		{
			m_CommandBuffer = VulkanContext::GetCurrentVkCmdBuffer();
			m_IsMainCmdBufferInUse = true;
			return;
		}

		VulkanCommandBuffer::CreateCommandBuffer(&m_CmdStorage);
		m_CommandBuffer = m_CmdStorage.Buffer;
#else
		OpenglRendererAPI* instance = m_GraphicsContext->GetOpenglRendererAPI();
		instance->Init();
#endif
	}

	void GraphicsPipeline::EndCommandBuffer()
	{
#ifndef FROSTIUM_OPENGL_IMPL
		if (!m_IsMainCmdBufferInUse)
		{
			VulkanCommandBuffer::ExecuteCommandBuffer(&m_CmdStorage);
		}
#endif
	}

	void GraphicsPipeline::ResetStates()
	{
		m_DescriptorIndex = 0;
		m_DrawMode = DrawMode::Triangle;
	}

	void GraphicsPipeline::DrawIndexed(uint32_t vertexBufferIndex, uint32_t indexBufferIndex)
	{
#ifdef FROSTIUM_OPENGL_IMPL
		m_VextexArray->Bind();
		m_VextexArray->SetVertexBuffer(m_VertexBuffers[vertexBufferIndex]);
		m_VextexArray->SetIndexBuffer(m_IndexBuffers[indexBufferIndex]);
		m_Shader->Bind();

		OpenglRendererAPI* instance = m_GraphicsContext->GetOpenglRendererAPI();
		switch (m_DrawMode)
		{
		case Frostium::DrawMode::Triangle:
			instance->DrawTriangle(m_VextexArray);
			break;
		case Frostium::DrawMode::Line:
			instance->DrawLine(m_VextexArray);
			break;
		case Frostium::DrawMode::Fan:
			instance->DrawFan(m_VextexArray);
			break;
		default:
			break;
		}
#else
		vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_VulkanPipeline.GetVkPipeline(m_DrawMode));

		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(m_CommandBuffer, 0, 1, &m_VertexBuffers[vertexBufferIndex]->GetVulkanVertexBuffer().GetBuffer(), offsets);

		vkCmdBindIndexBuffer(m_CommandBuffer, m_IndexBuffers[indexBufferIndex]->GetVulkanIndexBuffer().GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

		const auto& descriptorSets = m_VulkanPipeline.GetVkDescriptorSets(m_DescriptorIndex);
		vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_VulkanPipeline.GetVkPipelineLayot(), 0, 1,
			&descriptorSets, 0, nullptr);

		vkCmdDrawIndexed(m_CommandBuffer, m_IndexBuffers[indexBufferIndex]->GetVulkanIndexBuffer().GetCount(), 1, 0, 0, 1);
#endif
	}

	void GraphicsPipeline::DrawIndexed(VertexBuffer* vb, IndexBuffer* ib)
	{
#ifdef FROSTIUM_OPENGL_IMPL
#else
		vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_VulkanPipeline.GetVkPipeline(m_DrawMode));

		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(m_CommandBuffer, 0, 1, &vb->GetVulkanVertexBuffer().GetBuffer(), offsets);

		vkCmdBindIndexBuffer(m_CommandBuffer, ib->GetVulkanIndexBuffer().GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

		const auto& descriptorSets = m_VulkanPipeline.GetVkDescriptorSets(m_DescriptorIndex);
		vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_VulkanPipeline.GetVkPipelineLayot(), 0, 1,
			&descriptorSets, 0, nullptr);

		vkCmdDrawIndexed(m_CommandBuffer, ib->GetVulkanIndexBuffer().GetCount(), 1, 0, 0, 1);
#endif
	}

	void GraphicsPipeline::Draw(VertexBuffer* vb, uint32_t vertextCount)
	{
#ifdef FROSTIUM_OPENGL_IMPL
#else
		vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_VulkanPipeline.GetVkPipeline(m_DrawMode));

		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(m_CommandBuffer, 0, 1, &vb->GetVulkanVertexBuffer().GetBuffer(), offsets);

		const auto& descriptorSets = m_VulkanPipeline.GetVkDescriptorSets(m_DescriptorIndex);
		vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_VulkanPipeline.GetVkPipelineLayot(), 0, 1,
			&descriptorSets, 0, nullptr);

		vkCmdDraw(m_CommandBuffer, vertextCount, 1, 0, 0);
#endif
	}

	void GraphicsPipeline::Draw(uint32_t vertextCount, uint32_t vertexBufferIndex)
	{
#ifdef FROSTIUM_OPENGL_IMPL
		m_VextexArray->Bind();
		m_VextexArray->SetVertexBuffer(m_VertexBuffers[vertexBufferIndex]);
		m_Shader->Bind();

		OpenglRendererAPI* instance = m_GraphicsContext->GetOpenglRendererAPI();
		switch (m_DrawMode)
		{
		case Frostium::DrawMode::Triangle:
			instance->DrawTriangle(m_VextexArray, 0, vertextCount);
			break;
		case Frostium::DrawMode::Line:
			instance->DrawLine(m_VextexArray, 0, vertextCount);
			break;
		case Frostium::DrawMode::Fan:
			instance->DrawFan(m_VextexArray, 0, vertextCount);
			break;
		default:
			break;
		}
#else
		vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_VulkanPipeline.GetVkPipeline(m_DrawMode));

		VkDeviceSize offsets[1] = { 0 };
		if (m_VertexBuffers.size() > 0)
		{
			vkCmdBindVertexBuffers(m_CommandBuffer, 0, 1, &m_VertexBuffers[vertexBufferIndex]->GetVulkanVertexBuffer().GetBuffer(), offsets);
		}

		const auto& descriptorSets = m_VulkanPipeline.GetVkDescriptorSets(m_DescriptorIndex);
		vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_VulkanPipeline.GetVkPipelineLayot(), 0, 1,
			&descriptorSets, 0, nullptr);

		vkCmdDraw(m_CommandBuffer, vertextCount, 1, 0, 0);
#endif
	}

	void GraphicsPipeline::DrawMeshIndexed(Mesh* mesh, uint32_t instances)
	{
#ifdef FROSTIUM_OPENGL_IMPL
#else
		vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_VulkanPipeline.GetVkPipeline(m_DrawMode));

		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(m_CommandBuffer, 0, 1, &mesh->GetVertexBuffer()->GetVulkanVertexBuffer().GetBuffer(), offsets);
		vkCmdBindIndexBuffer(m_CommandBuffer, mesh->GetIndexBuffer()->GetVulkanIndexBuffer().GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

		const auto& descriptorSets = m_VulkanPipeline.GetVkDescriptorSets(m_DescriptorIndex);
		vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_VulkanPipeline.GetVkPipelineLayot(), 0, 1,
			&descriptorSets, 0, nullptr);

		vkCmdDrawIndexed(m_CommandBuffer, mesh->GetIndexBuffer()->GetCount(), instances, 0, 0, 0);
#endif
	}

	void GraphicsPipeline::DrawMesh(Mesh* mesh, uint32_t instances)
	{
#ifdef FROSTIUM_OPENGL_IMPL
#else
		vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_VulkanPipeline.GetVkPipeline(m_DrawMode));

		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(m_CommandBuffer, 0, 1, &mesh->GetVertexBuffer()->GetVulkanVertexBuffer().GetBuffer(), offsets);
		vkCmdBindIndexBuffer(m_CommandBuffer, mesh->GetIndexBuffer()->GetVulkanIndexBuffer().GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

		const auto& descriptorSets = m_VulkanPipeline.GetVkDescriptorSets(m_DescriptorIndex);
		vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_VulkanPipeline.GetVkPipelineLayot(), 0, 1,
			&descriptorSets, 0, nullptr);

		vkCmdDraw(m_CommandBuffer, mesh->m_VertexCount, instances, 0, 0);
#endif
	}

	void GraphicsPipeline::SubmitPushConstant(ShaderType shaderStage, size_t size, const void* data)
	{
#ifndef FROSTIUM_OPENGL_IMPL
		vkCmdPushConstants(m_CommandBuffer, m_VulkanPipeline.GetVkPipelineLayot(), VulkanShader::GetVkShaderStage(shaderStage),
			0, static_cast<uint32_t>(size), data);
#endif
	}

	void GraphicsPipeline::SetVertexBuffers(const std::vector<Ref<VertexBuffer>>& buffer)
	{
		m_VertexBuffers = buffer;
	}

	void GraphicsPipeline::SetIndexBuffers(const std::vector<Ref<IndexBuffer>>& buffer)
	{
		m_IndexBuffers = buffer;
	}

#ifndef FROSTIUM_OPENGL_IMPL
	void GraphicsPipeline::CmdUpdateVertextBuffer(const void* data, size_t size, uint32_t bufferIndex, uint32_t offset)
	{
		m_VertexBuffers[bufferIndex]->CmdUpdateData(m_CommandBuffer, data, size, offset);
	}

	void GraphicsPipeline::CmdUpdateIndexBuffer(uint32_t* indices, size_t count, uint32_t bufferIndex, uint32_t offset)
	{
		m_IndexBuffers[bufferIndex]->CmdUpdateData(m_CommandBuffer, indices, sizeof(uint32_t) * count, offset);
	}

	bool GraphicsPipeline::UpdateVulkanImageDescriptor(uint32_t bindingPoint, const VkDescriptorImageInfo& imageInfo)
	{
		return m_VulkanPipeline.m_Descriptors[m_DescriptorIndex].UpdateImageResource(bindingPoint, imageInfo);
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

	const VkPipeline& GraphicsPipeline::GetVkPipeline(DrawMode mode)
	{
		return m_VulkanPipeline.GetVkPipeline(mode);
	}

	const VulkanShader* GraphicsPipeline::GetVulkanShader()
	{
		return m_Shader->GetVulkanShader();
	}

	VkCommandBuffer GraphicsPipeline::GetVkCommandBuffer() const
	{
		return m_CommandBuffer;
	}

	void GraphicsPipeline::SetCommandBuffer(VkCommandBuffer cmdBuffer)
	{
		m_CommandBuffer = cmdBuffer;
	}
#else
	void GraphicsPipeline::BindOpenGLShader()
	{
		m_Shader->Bind();
	}
#endif

	bool GraphicsPipeline::UpdateSamplers(const std::vector<Texture*>& textures, uint32_t bindingPoint)
	{
		m_Shader->Bind();
		uint32_t index = 0;
#ifndef FROSTIUM_OPENGL_IMPL
		std::vector<VulkanTexture*> vkTextures(textures.size());
#endif
		for (const auto& tex : textures)
		{
			if (tex == nullptr)
			{
				break;
			}

#ifdef FROSTIUM_OPENGL_IMPL
			tex->Bind(index);
#else
			vkTextures[index] = tex->GetVulkanTexture();
#endif
			index++;
		}

#ifndef FROSTIUM_OPENGL_IMPL
		return m_VulkanPipeline.UpdateSamplers2D(vkTextures, bindingPoint, m_DescriptorIndex);
#else
		return true;
#endif
	}

	bool GraphicsPipeline::UpdateSampler(Texture* tetxure, uint32_t bindingPoint)
	{
#ifdef FROSTIUM_OPENGL_IMPL
		return false;
#else
		return m_VulkanPipeline.m_Descriptors[m_DescriptorIndex].UpdateImageResource(bindingPoint,
			tetxure->GetVulkanTexture()->GetVkDescriptorImageInfo());
#endif
	}

	bool GraphicsPipeline::UpdateSampler(Framebuffer* framebuffer, uint32_t bindingPoint, uint32_t attachmentIndex)
	{
#ifdef FROSTIUM_OPENGL_IMPL
		return false;
#else
		const auto& descriptor = framebuffer->GetVulkanFramebuffer().GetAttachment(attachmentIndex)->ImageInfo;
		return m_VulkanPipeline.m_Descriptors[m_DescriptorIndex].UpdateImageResource(bindingPoint, descriptor);
#endif
	}

	bool GraphicsPipeline::UpdateSampler(Framebuffer* framebuffer, uint32_t bindingPoint, const std::string& attachmentName)
	{
#ifdef FROSTIUM_OPENGL_IMPL
		return false;
#else
		const auto& descriptor = framebuffer->GetVulkanFramebuffer().GetAttachment(attachmentName)->ImageInfo;
		return m_VulkanPipeline.m_Descriptors[m_DescriptorIndex].UpdateImageResource(bindingPoint, descriptor);
#endif
	}

	bool GraphicsPipeline::UpdateCubeMap(Texture* cubeMap, uint32_t bindingPoint)
	{
#ifdef FROSTIUM_OPENGL_IMPL
		return false; // temp
#else
		return m_VulkanPipeline.UpdateCubeMap(cubeMap->GetVulkanTexture(), bindingPoint, m_DescriptorIndex);
#endif
	}

	bool GraphicsPipeline::IsPipelineCreateInfoValid(const GraphicsPipelineCreateInfo* pipelineInfo)
	{
		if (pipelineInfo->DescriptorSets < 1 || pipelineInfo->TargetFramebuffers.size() == 0)
			return false;

		return true;
	}
}