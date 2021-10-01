#include "stdafx.h"
#ifndef OPENGL_IMPL
#include "Backends/Vulkan/VulkanPipeline.h"
#include "Backends/Vulkan/VulkanRenderPass.h"
#include "Backends/Vulkan/VulkanShader.h"
#include "Backends/Vulkan/VulkanContext.h"
#include "Backends/Vulkan/VulkanTexture.h"

#include "Primitives/GraphicsPipeline.h"
#include "Primitives/Framebuffer.h"

namespace SmolEngine
{
	bool VulkanPipeline::CreatePipeline(DrawMode mode)
	{
		VulkanShader* shader = m_Shader->Cast<VulkanShader>();
		Framebuffer* fb = m_PiplineCreateInfo.TargetFramebuffers[0];
		// Create the graphics pipeline
		// Vulkan uses the concept of rendering pipelines to encapsulate fixed states, replacing OpenGL's complex state machine
		// A pipeline is then stored and hashed on the GPU making pipeline changes very fast
		// Note: There are still a few dynamic states that are not directly part of the pipeline (but the info that they are used is)

		VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
		{
			pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			// The layout used for this pipeline (can be shared among multiple pipelines using the same layout)
			pipelineCreateInfo.layout = m_PipelineLayout;
			// Renderpass this pipeline is attached to
			pipelineCreateInfo.renderPass = m_TargetRenderPass;
		}

		// Construct the different states making up the pipeline

		// Input assembly state describes how primitives are assembled
		// This pipeline will assemble vertex data as a triangle lists (though we only use one triangle)
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
		{
			inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssemblyState.topology = GetVkTopology(mode);
			inputAssemblyState.primitiveRestartEnable = m_PiplineCreateInfo.bPrimitiveRestartEnable;
		}

		// Rasterization state
		VkPipelineRasterizationStateCreateInfo rasterizationState = {};
		{
			rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizationState.polygonMode = GetVkPolygonMode(m_PiplineCreateInfo.ePolygonMode);
			rasterizationState.cullMode = GetVkCullMode(m_PiplineCreateInfo.eCullMode);
			rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
			rasterizationState.depthClampEnable = VulkanContext::GetDevice().GetDeviceFeatures()->depthClamp;
			rasterizationState.rasterizerDiscardEnable = VK_FALSE;
			rasterizationState.depthBiasEnable = m_PiplineCreateInfo.bDepthBiasEnabled;
			rasterizationState.lineWidth = 1.0f;
		}

		// Color blend state describes how blend factors are calculated (if used)
		// We need one blend attachment state per color attachment (even if blending is not used)
		std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentState;
		{
			uint32_t count = static_cast<uint32_t>(fb->GetSpecification().Attachments.size());
			blendAttachmentState.resize(count);

			for (uint32_t i = 0; i < count; ++i)
			{
				blendAttachmentState[i].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
				blendAttachmentState[i].blendEnable = IsBlendEnableEnabled() ? VK_TRUE: VK_FALSE;
				blendAttachmentState[i].srcColorBlendFactor = GetVkBlendFactor(m_PiplineCreateInfo.eSrcColorBlendFactor);
				blendAttachmentState[i].dstColorBlendFactor = GetVkBlendFactor(m_PiplineCreateInfo.eDstColorBlendFactor);;
				blendAttachmentState[i].colorBlendOp = GetVkBlendOp(m_PiplineCreateInfo.eColorBlendOp);
				blendAttachmentState[i].srcAlphaBlendFactor = GetVkBlendFactor(m_PiplineCreateInfo.eSrcAlphaBlendFactor);
				blendAttachmentState[i].dstAlphaBlendFactor = GetVkBlendFactor(m_PiplineCreateInfo.eDstAlphaBlendFactor);
				blendAttachmentState[i].alphaBlendOp = GetVkBlendOp(m_PiplineCreateInfo.eAlphaBlendOp);
			}
		}

		VkPipelineColorBlendStateCreateInfo colorBlendState = {};
		{
			colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			colorBlendState.attachmentCount = static_cast<uint32_t>(blendAttachmentState.size());
			colorBlendState.pAttachments = blendAttachmentState.data();
		}

		// Viewport state sets the number of viewports and scissor used in this pipeline
		// Note: This is actually overridden by the dynamic states (see below)
		VkPipelineViewportStateCreateInfo viewportState = {};
		{
			viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportState.viewportCount = 1;
			viewportState.scissorCount = 1;
		}

		// Enable dynamic states
		// Most states are baked into the pipeline, but there are still a few dynamic states that can be changed within a command buffer
		// To be able to change these we need do specify which dynamic states will be changed using this pipeline. Their actual states are set later on in the command buffer.
		// For this example we will set the viewport and scissor using dynamic states
		VkPipelineDynamicStateCreateInfo dynamicState = {};
		std::vector<VkDynamicState> dynamicStateEnables;
		{
			dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);
			dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);

			if(m_PiplineCreateInfo.bDepthBiasEnabled)
				dynamicStateEnables.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);


			dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicState.pDynamicStates = dynamicStateEnables.data();
			dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
		}

		// Depth and stencil state containing depth and stencil compare and test operations
		// We only use depth tests and want depth tests and writes to be enabled and compare with less or equal
		VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
		{
			depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depthStencilState.depthTestEnable = m_PiplineCreateInfo.bDepthTestEnabled;
			depthStencilState.depthWriteEnable = m_PiplineCreateInfo.bDepthWriteEnabled;
			depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
			depthStencilState.depthBoundsTestEnable = VK_FALSE;
			depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;
			depthStencilState.stencilTestEnable = VK_FALSE;
			depthStencilState.minDepthBounds = m_PiplineCreateInfo.MinDepth;
			depthStencilState.maxDepthBounds = m_PiplineCreateInfo.MaxDepth;
		}

		// Multi sampling state
		VkPipelineMultisampleStateCreateInfo multisampleState = {};
		{
			multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

			if (fb->GetSpecification().eMSAASampels != MSAASamples::SAMPLE_COUNT_1)
			{
#ifndef SMOLEGNINE_OPENGL_IMPL
				multisampleState.rasterizationSamples = fb->GetVulkanFramebuffer().GetMSAASamples();
#endif
				multisampleState.sampleShadingEnable = VK_TRUE;
				multisampleState.minSampleShading = 0.2f;
				multisampleState.pSampleMask = nullptr;
			}
		}

		std::vector<VkVertexInputBindingDescription> vertexInputBindings(m_PiplineCreateInfo.VertexInputInfos.size());
		std::vector<VkVertexInputAttributeDescription> vertexInputAttributs;
		{
			uint32_t index = 0;
			uint32_t location = 0;
			for (const auto& inputInfo : m_PiplineCreateInfo.VertexInputInfos)
			{
				// Vertex input binding
				// This example uses a single vertex input binding at binding point 0 (see vkCmdBindVertexBuffers)
				{
					vertexInputBindings[index].binding = index;
					vertexInputBindings[index].stride = inputInfo.Stride;
					vertexInputBindings[index].inputRate = inputInfo.IsInputRateInstance ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
				}

				// Vertex input descriptions
				// Specifies the vertex input parameters for a pipeline
				{

					for (const auto& element : inputInfo.Layout.GetElements())
					{
						if (element.type == DataTypes::Mat3 || element.type == DataTypes::Mat4)
						{
							uint32_t count = 0;
							uint32_t offset = vertexInputAttributs[location - 1].offset;
							element.type == DataTypes::Mat3 ? count = 3 : count = 4;

							for (uint32_t i = 0; i < count; ++i)
							{
								offset += count * 4;
								VkVertexInputAttributeDescription inputAttributeDescription;
								{
									inputAttributeDescription.binding = index;
									inputAttributeDescription.location = location;
									inputAttributeDescription.format = GetVkInputFormat(element.type);
									inputAttributeDescription.offset = offset;
								}

								vertexInputAttributs.emplace_back(inputAttributeDescription);
								location++;
							}

							continue;
						}

						VkVertexInputAttributeDescription inputAttributeDescription;
						{
							inputAttributeDescription.binding = index;
							inputAttributeDescription.location = location;
							inputAttributeDescription.format = GetVkInputFormat(element.type);
							inputAttributeDescription.offset = element.offset;
						}

						vertexInputAttributs.emplace_back(inputAttributeDescription);
						location++;
					}
				}

				index++;
			}
		}

		// Vertex input state used for pipeline creation
		VkPipelineVertexInputStateCreateInfo vertexInputState = {};
		{
			vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			if (m_PiplineCreateInfo.VertexInputInfos.size() > 0)
			{
				vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size());
				vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
				vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributs.size());
				vertexInputState.pVertexAttributeDescriptions = vertexInputAttributs.data();
			}
		}

		// Set pipeline shader stage info
		pipelineCreateInfo.stageCount = m_PiplineCreateInfo.StageCount == -1? static_cast<uint32_t>(shader->GetVkPipelineShaderStages().size()): m_PiplineCreateInfo.StageCount;
		pipelineCreateInfo.pStages = shader->GetVkPipelineShaderStages().data();

		// Assign the pipeline states to the pipeline creation info structure
		pipelineCreateInfo.pVertexInputState = &vertexInputState;
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineCreateInfo.pRasterizationState = &rasterizationState;
		pipelineCreateInfo.pColorBlendState = &colorBlendState;
		pipelineCreateInfo.pMultisampleState = &multisampleState;
		pipelineCreateInfo.pViewportState = &viewportState;
		pipelineCreateInfo.pDepthStencilState = &depthStencilState;
		pipelineCreateInfo.pDynamicState = &dynamicState;

		std::string name = std::string(m_FilePath + "_pipeline_" + std::to_string(m_PipelineCaches.size()) + ".cached");
		bool is_loaded = CreateOrLoadCached(name, mode);

		// Create rendering pipeline using the specified states
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(m_Device, m_PipelineCaches[mode], 1, &pipelineCreateInfo, nullptr, &m_Pipelines[mode]));
		if (!is_loaded)
		{
			SaveCache(name, mode);
		}

		return true;
	}

	VulkanPipeline::~VulkanPipeline()
	{
		Destroy();
	}

	bool VulkanPipeline::Build(GraphicsPipelineCreateInfo* pipelineInfo)
	{
		if (BuildBase(pipelineInfo))
		{
			m_Device = VulkanContext::GetDevice().GetLogicalDevice();
			BuildDescriptors(m_Shader, pipelineInfo->DescriptorSets, m_Descriptors, m_DescriptorPool);

			VulkanShader* shader = m_Shader->Cast<VulkanShader>();

#ifndef OPENGL_IMPL
			Framebuffer* fb = pipelineInfo->TargetFramebuffers[0];
			m_TargetRenderPass = fb->GetVulkanFramebuffer().GetRenderPass();
#endif
			m_SetLayout.clear();
			m_SetLayout.reserve(m_Descriptors.size());
			for (auto& descriptor : m_Descriptors)
			{
				m_SetLayout.push_back(descriptor.m_DescriptorSetLayout);
			}

			VkPipelineLayoutCreateInfo pipelineLayoutCI = {};
			{
				pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
				pipelineLayoutCI.pNext = nullptr;
				pipelineLayoutCI.setLayoutCount = static_cast<uint32_t>(m_SetLayout.size());
				pipelineLayoutCI.pSetLayouts = m_SetLayout.data();
				pipelineLayoutCI.pushConstantRangeCount = static_cast<uint32_t>(shader->m_VkPushConstantRanges.size());
				pipelineLayoutCI.pPushConstantRanges = shader->m_VkPushConstantRanges.data();

				VK_CHECK_RESULT(vkCreatePipelineLayout(m_Device, &pipelineLayoutCI, nullptr, &m_PipelineLayout));
			}

			m_FilePath = "../resources/PipelineCache/" + pipelineInfo->PipelineName;

			for (DrawMode mode : pipelineInfo->PipelineDrawModes)
			{
				if (!CreatePipeline(mode)) { return false; }
			}

			shader->DeleteShaderModules();
			return true;
		}

		return false;
	}

	void VulkanPipeline::ClearColors(const glm::vec4& clearColors)
	{
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
	}

	void VulkanPipeline::BeginRenderPass(bool flip)
	{
		Framebuffer* target_fb = m_PiplineCreateInfo.TargetFramebuffers[m_FBIndex];
		auto& vulkanFB = target_fb->GetVulkanFramebuffer();
		const VkFramebuffer vkFramebuffer = m_FBattachmentIndex == 0 ? vulkanFB.GetCurrentVkFramebuffer() : vulkanFB.GetVkFramebuffer(m_FBattachmentIndex);
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
	}

	void VulkanPipeline::EndRenderPass()
	{
		vkCmdEndRenderPass(m_CommandBuffer);
	}

	void VulkanPipeline::BeginCommandBuffer(bool isMainCmdBufferInUse)
	{
		if (isMainCmdBufferInUse)
		{
			m_CommandBuffer = VulkanContext::GetCurrentVkCmdBuffer();
			m_IsMainCmdBufferInUse = true;
			return;
		}

		VulkanCommandBuffer::CreateCommandBuffer(&m_CmdStorage);
		m_CommandBuffer = m_CmdStorage.Buffer;
	}

	void VulkanPipeline::EndCommandBuffer()
	{
		if (!m_IsMainCmdBufferInUse)
			VulkanCommandBuffer::ExecuteCommandBuffer(&m_CmdStorage);
	}

	void VulkanPipeline::Destroy()
	{
		for (auto& [key, cache] : m_PipelineCaches)
		{
			vkDestroyPipelineCache(m_Device, cache, nullptr);
		}

		for (auto& [key, pipeline] : m_Pipelines)
		{
			vkDestroyPipeline(m_Device, pipeline, nullptr);
		}

		vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
		m_Pipelines.clear();
		m_PipelineCaches.clear();
	}

	void VulkanPipeline::Reload()
	{

	}

	void VulkanPipeline::DrawIndexed(uint32_t vbIndex, uint32_t ibIndex)
	{
		vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, GetVkPipeline(m_DrawMode));

		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(m_CommandBuffer, 0, 1, &m_VertexBuffers[vbIndex]->GetVulkanVertexBuffer().GetBuffer(), offsets);

		vkCmdBindIndexBuffer(m_CommandBuffer, m_IndexBuffers[ibIndex]->GetVulkanIndexBuffer().GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

		const auto& descriptorSets = GetVkDescriptorSets(m_DescriptorIndex);
		vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &descriptorSets, 0, nullptr);
		vkCmdDrawIndexed(m_CommandBuffer, m_IndexBuffers[ibIndex]->GetVulkanIndexBuffer().GetElementsCount(), 1, 0, 0, 1);
	}

	void VulkanPipeline::DrawIndexed(VertexBuffer* vb, IndexBuffer* ib)
	{
		vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, GetVkPipeline(m_DrawMode));

		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(m_CommandBuffer, 0, 1, &vb->GetVulkanVertexBuffer().GetBuffer(), offsets);

		vkCmdBindIndexBuffer(m_CommandBuffer, ib->GetVulkanIndexBuffer().GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

		const auto& descriptorSets = GetVkDescriptorSets(m_DescriptorIndex);
		vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &descriptorSets, 0, nullptr);
		vkCmdDrawIndexed(m_CommandBuffer, ib->GetVulkanIndexBuffer().GetElementsCount(), 1, 0, 0, 1);
	}

	void VulkanPipeline::Draw(VertexBuffer* vb, uint32_t vertextCount)
	{
		vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, GetVkPipeline(m_DrawMode));

		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(m_CommandBuffer, 0, 1, &vb->GetVulkanVertexBuffer().GetBuffer(), offsets);

		const auto& descriptorSets = GetVkDescriptorSets(m_DescriptorIndex);
		vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &descriptorSets, 0, nullptr);

		vkCmdDraw(m_CommandBuffer, vertextCount, 1, 0, 0);
	}

	void VulkanPipeline::Draw(uint32_t vertextCount, uint32_t vertexBufferIndex)
	{
		vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, GetVkPipeline(m_DrawMode));

		VkDeviceSize offsets[1] = { 0 };
		if (m_VertexBuffers.size() > 0)
		{
			vkCmdBindVertexBuffers(m_CommandBuffer, 0, 1, &m_VertexBuffers[vertexBufferIndex]->GetVulkanVertexBuffer().GetBuffer(), offsets);
		}

		const auto& descriptorSets = GetVkDescriptorSets(m_DescriptorIndex);
		vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &descriptorSets, 0, nullptr);
		vkCmdDraw(m_CommandBuffer, vertextCount, 1, 0, 0);
	}

	void VulkanPipeline::DrawMeshIndexed(Mesh* mesh, uint32_t instances)
	{
		vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,GetVkPipeline(m_DrawMode));

		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(m_CommandBuffer, 0, 1, &mesh->GetVertexBuffer()->GetVulkanVertexBuffer().GetBuffer(), offsets);
		vkCmdBindIndexBuffer(m_CommandBuffer, mesh->GetIndexBuffer()->GetVulkanIndexBuffer().GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

		const auto& descriptorSets = GetVkDescriptorSets(m_DescriptorIndex);
		vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &descriptorSets, 0, nullptr);
		vkCmdDrawIndexed(m_CommandBuffer, mesh->GetIndexBuffer()->GetCount(), instances, 0, 0, 0);
	}

	void VulkanPipeline::DrawMesh(Mesh* mesh, uint32_t instances)
	{
		vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, GetVkPipeline(m_DrawMode));

		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(m_CommandBuffer, 0, 1, &mesh->GetVertexBuffer()->GetVulkanVertexBuffer().GetBuffer(), offsets);
		vkCmdBindIndexBuffer(m_CommandBuffer, mesh->GetIndexBuffer()->GetVulkanIndexBuffer().GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

		const auto& descriptorSets =GetVkDescriptorSets(m_DescriptorIndex);
		vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &descriptorSets, 0, nullptr);
		vkCmdDraw(m_CommandBuffer, mesh->GetVertexCount(), instances, 0, 0);
	}

	bool VulkanPipeline::SubmitBuffer(uint32_t bindingPoint, size_t size, const void* data, uint32_t offset)
	{
		return m_Descriptors[m_DescriptorIndex].UpdateBuffer(bindingPoint, size, data, offset);
	}

	void VulkanPipeline::SubmitPushConstant(ShaderType shaderStage, size_t size, const void* data)
	{
		vkCmdPushConstants(m_CommandBuffer, m_PipelineLayout, VulkanShader::GetVkShaderStage(shaderStage), 0, static_cast<uint32_t>(size), data);
	}

	bool VulkanPipeline::UpdateSamplers(const std::vector<Ref<Texture>>& textures, uint32_t bindingPoint, bool storageImage)
	{
		return m_Descriptors[m_DescriptorIndex].Update2DSamplers(textures, bindingPoint, storageImage);
	}

	bool VulkanPipeline::UpdateSampler(Ref<Texture>& tetxure, uint32_t bindingPoint, bool storageImage)
	{
		return m_Descriptors[m_DescriptorIndex].UpdateImageResource(bindingPoint, tetxure->Cast<VulkanTexture>()->GetVkDescriptorImageInfo());
	}

	bool VulkanPipeline::UpdateSampler(Framebuffer* framebuffer, uint32_t bindingPoint, uint32_t attachmentIndex)
	{
		const auto& descriptor = framebuffer->GetVulkanFramebuffer().GetAttachment(attachmentIndex)->ImageInfo;
		return m_Descriptors[m_DescriptorIndex].UpdateImageResource(bindingPoint, descriptor);
	}

	bool VulkanPipeline::UpdateSampler(Framebuffer* framebuffer, uint32_t bindingPoint, const std::string& attachmentName)
	{
		const auto& descriptor = framebuffer->GetVulkanFramebuffer().GetAttachment(attachmentName)->ImageInfo;
		return m_Descriptors[m_DescriptorIndex].UpdateImageResource(bindingPoint, descriptor);
	}

	bool VulkanPipeline::UpdateCubeMap(Ref<Texture>& cubeMap, uint32_t bindingPoint)
	{
		return m_Descriptors[m_DescriptorIndex].UpdateCubeMap(cubeMap, bindingPoint);
	}

	void VulkanPipeline::UpdateImageDescriptor(uint32_t bindingPoint, const VkDescriptorImageInfo& imageInfo)
	{
		m_Descriptors[m_DescriptorIndex].UpdateImageResource(bindingPoint, imageInfo);
	}

	void VulkanPipeline::BindPipeline()
	{
		vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, GetVkPipeline(m_DrawMode));
	}

	void VulkanPipeline::BindDescriptors()
	{
		const auto& descriptorSets = GetVkDescriptorSets(m_DescriptorIndex);
		vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &descriptorSets, 0, nullptr);
	}

	void VulkanPipeline::BindIndexBuffer(uint32_t index)
	{
		vkCmdBindIndexBuffer(m_CommandBuffer, m_IndexBuffers[index]->GetVulkanIndexBuffer().GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
	}

	void VulkanPipeline::BindVertexBuffer(uint32_t index)
	{
		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(m_CommandBuffer, 0, 1, &m_VertexBuffers[index]->GetVulkanVertexBuffer().GetBuffer(), offsets);
	}

	bool VulkanPipeline::SaveCache(const std::string& fileName, DrawMode mode)
	{
		FILE* f = fopen(fileName.c_str(), "wb");
		if (f)
		{
			size_t size = 0;
			void* data = nullptr;

			vkGetPipelineCacheData(m_Device, m_PipelineCaches[mode], &size, nullptr);
			data = (char*)malloc(sizeof(char) * size);
			vkGetPipelineCacheData(m_Device, m_PipelineCaches[mode], &size, data);

			size_t result = fwrite(data, sizeof(char), size, f);
			if (result != size)
			{
				DebugLog::LogError("VulkanPipeline::SaveCache(): cache was not saved");
				fclose(f);
				free(data);
				return false;
			}

			fclose(f);
			free(data);
			return true;
		}

		return false;
	}

	bool VulkanPipeline::CreateOrLoadCached(const std::string& fileName, DrawMode mode)
	{
		VkPipelineCache cache = nullptr;

		FILE* f = fopen(fileName.c_str(), "rb");
		if (f)
		{
			fseek(f, 0, SEEK_END);
			size_t size = ftell(f);
			rewind(f);
			if (size != 0)
			{
				void* data = (char*)malloc(sizeof(char) * size);
				fread(data, 1, size, f);
				fclose(f);

				VkPipelineCacheCreateInfo pipelineCacheCI = {};
				{
					pipelineCacheCI.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
					pipelineCacheCI.initialDataSize = size;
					pipelineCacheCI.pInitialData = data;
				}

				VK_CHECK_RESULT(vkCreatePipelineCache(m_Device, &pipelineCacheCI, nullptr, &cache));
				m_PipelineCaches[mode] = cache;
				free(data);
				return true;
			}
		}

		VkPipelineCacheCreateInfo pipelineCacheCI = {};
		{
			pipelineCacheCI.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		}

		VK_CHECK_RESULT(vkCreatePipelineCache(m_Device, &pipelineCacheCI, nullptr, &cache));
		m_PipelineCaches[mode] = cache;
		return false;
	}

	const VkPipeline& VulkanPipeline::GetVkPipeline(DrawMode mode)
	{
		return m_Pipelines[mode];
	}

	const VkPipelineLayout& VulkanPipeline::GetVkPipelineLayot() const
	{
		return m_PipelineLayout;
	}

	VkCommandBuffer VulkanPipeline::GetCommandBuffer()
	{
		return m_CommandBuffer;
	}

	const VkDescriptorSet VulkanPipeline::GetVkDescriptorSets(uint32_t setIndex) const
	{
		return m_Descriptors[setIndex].GetDescriptorSets();
	}

	void VulkanPipeline::SetCommandBuffer(VkCommandBuffer cmd)
	{
		m_CommandBuffer = cmd;
	}

	bool VulkanPipeline::IsBlendEnableEnabled()
	{
		return m_PiplineCreateInfo.eSrcColorBlendFactor != BlendFactor::NONE || m_PiplineCreateInfo.eDstColorBlendFactor != BlendFactor::NONE ||
			m_PiplineCreateInfo.eDstAlphaBlendFactor != BlendFactor::NONE || m_PiplineCreateInfo.eSrcAlphaBlendFactor != BlendFactor::NONE;
	}

	void VulkanPipeline::BuildDescriptors(Ref<Shader>& shader, uint32_t DescriptorSets, std::vector<VulkanDescriptor>& out_descriptors, VkDescriptorPool& pool)
	{
		auto device = VulkanContext::GetDevice().GetLogicalDevice();

		const ReflectionData& refData = shader->GetReflection();
		std::vector< VkDescriptorPoolSize> DescriptorPoolSizes;
		if (refData.Buffers.size() > 0)
		{
			uint32_t UBOcount = 0;
			uint32_t SSBOcount = 0;

			for (auto& [binding, buffer] : refData.Buffers)
			{
				if (buffer.Type == BufferType::Uniform)
					UBOcount++;
				else
					SSBOcount++;
			}

			if (UBOcount > 0)
			{
				VkDescriptorPoolSize poolSize = {};
				{
					poolSize.descriptorCount = UBOcount;
					poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				}

				DescriptorPoolSizes.push_back(poolSize);
			}

			if (SSBOcount > 0)
			{
				VkDescriptorPoolSize poolSize = {};
				{
					poolSize.descriptorCount = SSBOcount;
					poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				}

				DescriptorPoolSizes.push_back(poolSize);
			}
		}

		if(refData.ImageSamplers.size() > 0)
		{
			uint32_t samplerDescriptors = 0;
			for (auto& info : refData.ImageSamplers)
			{
				auto& [bindingPoint, res] = info;
				samplerDescriptors += res.ArraySize > 0 ? res.ArraySize : 1;
			}

			VkDescriptorPoolSize poolSize = {};
			{
				poolSize.descriptorCount = samplerDescriptors;
				poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			}

			DescriptorPoolSizes.push_back(poolSize);
		}

		if (refData.StorageImages.size() > 0)
		{
			uint32_t samplerDescriptors = 0;
			for (auto& info : refData.StorageImages)
			{
				auto& [bindingPoint, res] = info;
				samplerDescriptors += res.ArraySize > 0 ? res.ArraySize : 1;
			}

			VkDescriptorPoolSize poolSize = {};
			{
				poolSize.descriptorCount = samplerDescriptors;
				poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			}

			DescriptorPoolSizes.push_back(poolSize);
		}

		if (refData.ImageSamplers.size() == 0 && refData.Buffers.size() == 0
			&& refData.StorageImages.size() == 0)
		{
			// dummy
			VkDescriptorPoolSize poolSize = {};
			{
				poolSize.descriptorCount = 1;
				poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			}
			DescriptorPoolSizes.push_back(poolSize);
		}

		VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
		{
			descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			descriptorPoolInfo.pNext = nullptr;
			descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(DescriptorPoolSizes.size());
			descriptorPoolInfo.pPoolSizes = DescriptorPoolSizes.data();
			descriptorPoolInfo.maxSets = DescriptorSets;

			if (pool != VK_NULL_HANDLE) { vkDestroyDescriptorPool(device, pool, nullptr); }
			VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &pool));
		}

		out_descriptors.resize(DescriptorSets);
		for (uint32_t i = 0; i < DescriptorSets; ++i)
		{
			out_descriptors[i].GenDescriptorSet(shader, pool);
			out_descriptors[i].GenBuffersDescriptors(shader);
			out_descriptors[i].GenSamplersDescriptors(shader);
		}
	}

	VkFormat VulkanPipeline::GetVkInputFormat(DataTypes type)
	{
		switch (type)
		{
		case DataTypes::None:			                     break;
		case DataTypes::Float:	                             return VK_FORMAT_R32_SFLOAT;
		case DataTypes::Float2:		                         return VK_FORMAT_R32G32_SFLOAT;
		case DataTypes::Float3:		                         return VK_FORMAT_R32G32B32_SFLOAT;
		case DataTypes::Float4:		                         return VK_FORMAT_R32G32B32A32_SFLOAT;
		case DataTypes::Mat3:			                     return VK_FORMAT_R32G32B32_SFLOAT;
		case DataTypes::Mat4:			                     return VK_FORMAT_R32G32B32A32_SFLOAT;
		case DataTypes::Int:			                     return VK_FORMAT_R32_SINT;
		case DataTypes::Int2:			                     return VK_FORMAT_R32G32_SINT;
		case DataTypes::Int3:			                     return VK_FORMAT_R32G32B32_SINT;
		case DataTypes::Int4:			                     return VK_FORMAT_R32G32B32A32_SINT;
		case DataTypes::Bool:			                     return VK_FORMAT_R32_SINT;
		case DataTypes::Byte:			                     return VK_FORMAT_R8G8B8A8_UINT;
		}

		return VK_FORMAT_R32G32B32_SFLOAT;
	}

	VkPrimitiveTopology VulkanPipeline::GetVkTopology(DrawMode mode)
	{
		switch (mode)
		{
		case DrawMode::Triangle:	                           return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		case DrawMode::Line:			                       return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
		case DrawMode::Fan:			                           return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY;
		case DrawMode::Triangle_Strip:                         return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		default:			                                   return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		}
	}

	VkCullModeFlags VulkanPipeline::GetVkCullMode(CullMode mode)
	{
		switch (mode)
		{
		case CullMode::Back: 			                       return VK_CULL_MODE_BACK_BIT;
		case CullMode::Front: 		                           return VK_CULL_MODE_FRONT_BIT;
		case CullMode::None: 			                       return VK_CULL_MODE_NONE;
		default:			                                   return VK_CULL_MODE_BACK_BIT;
		}
	}

	VkPolygonMode VulkanPipeline::GetVkPolygonMode(PolygonMode mode)
	{
		switch (mode)
		{
		case PolygonMode::Fill:		                           return VkPolygonMode::VK_POLYGON_MODE_FILL;
		case PolygonMode::Line:				                   return VkPolygonMode::VK_POLYGON_MODE_LINE;
		case PolygonMode::Point:			                   return VkPolygonMode::VK_POLYGON_MODE_POINT;
		default:			                                   return VkPolygonMode::VK_POLYGON_MODE_FILL;
		}
	}

	VkBlendFactor VulkanPipeline::GetVkBlendFactor(BlendFactor factor)
	{
		switch (factor)
		{
		case BlendFactor::NONE:                                return VK_BLEND_FACTOR_ZERO;
		case BlendFactor::ONE:                                 return VK_BLEND_FACTOR_ONE;
		case BlendFactor::ZERO:                                return VK_BLEND_FACTOR_ZERO;
		case BlendFactor::SRC_ALPHA:                           return VK_BLEND_FACTOR_SRC_ALPHA;
		case BlendFactor::SRC_COLOR:                           return VK_BLEND_FACTOR_SRC_COLOR;
		case BlendFactor::ONE_MINUS_SRC_COLOR:                 return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		case BlendFactor::DST_COLOR:                           return VK_BLEND_FACTOR_DST_COLOR;
		case BlendFactor::ONE_MINUS_DST_COLOR:                 return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
		case BlendFactor::ONE_MINUS_SRC_ALPHA:                 return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		case BlendFactor::DST_ALPHA:                           return VK_BLEND_FACTOR_DST_ALPHA;
		case BlendFactor::ONE_MINUS_DST_ALPHA:                 return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
		case BlendFactor::CONSTANT_COLOR:                      return VK_BLEND_FACTOR_CONSTANT_COLOR;
		case BlendFactor::ONE_MINUS_CONSTANT_COLOR:            return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
		case BlendFactor::CONSTANT_ALPHA:                      return VK_BLEND_FACTOR_CONSTANT_ALPHA;
		case BlendFactor::ONE_MINUS_CONSTANT_ALPHA:            return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
		case BlendFactor::SRC_ALPHA_SATURATE:                  return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
		case BlendFactor::SRC1_COLOR:                          return VK_BLEND_FACTOR_SRC1_COLOR;
		case BlendFactor::ONE_MINUS_SRC1_COLOR:                return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
		case BlendFactor::SRC1_ALPHA:                          return VK_BLEND_FACTOR_SRC1_ALPHA;
		case BlendFactor::ONE_MINUS_SRC1_ALPHA:                return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
		default:                                               return VK_BLEND_FACTOR_ONE;
		}
	}

	VkBlendOp VulkanPipeline::GetVkBlendOp(BlendOp op)
	{
		switch (op)
		{
		case BlendOp::ADD:			                           return VK_BLEND_OP_ADD;
		case BlendOp::SUBTRACT:			                       return VK_BLEND_OP_SUBTRACT;
		case BlendOp::REVERSE_SUBTRACT:	                       return VK_BLEND_OP_REVERSE_SUBTRACT;
		case BlendOp::MIN:			                           return VK_BLEND_OP_MIN;
		case BlendOp::MAX:			                           return VK_BLEND_OP_MAX;
		default:			                                   return VK_BLEND_OP_ADD;
		}
	}
}
#endif