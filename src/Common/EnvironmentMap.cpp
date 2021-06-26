#include "stdafx.h"
#include "Common/EnvironmentMap.h"
#include "GraphicsContext.h"

#include "Vulkan/VulkanPBR.h"

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	void EnvironmentMap::Initialize()
	{
		std::string resPath = GraphicsContext::GetSingleton()->GetResourcesPath();

		// Framebuffer
		{
			FramebufferSpecification framebufferCI = {};
			framebufferCI.Width = 1024;
			framebufferCI.Height = 1024;
			framebufferCI.eMSAASampels = MSAASamples::SAMPLE_COUNT_1;
			framebufferCI.eSamplerFiltering = SamplerFilter::LINEAR;
			framebufferCI.eSpecialisation = FramebufferSpecialisation::CopyBuffer;
			framebufferCI.Attachments = { FramebufferAttachment(AttachmentFormat::Color, true) };

			Framebuffer::Create(framebufferCI, &m_Framebuffer);
		}

		// Pipeline
		{
			GraphicsPipelineCreateInfo pipelineCI = {};
			GraphicsPipelineShaderCreateInfo shaderCI = {};
			{
				shaderCI.FilePaths[ShaderType::Vertex] = resPath + "Shaders/DynamicSky.vert";
				shaderCI.FilePaths[ShaderType::Fragment] = resPath + "Shaders/DynamicSky.frag";
			};

			struct SkyBoxData
			{
				glm::vec3 pos;
			};

			BufferLayout layout =
			{
				{ DataTypes::Float3, "aPos" }
			};

			float skyboxVertices[] = {
				// positions          
				-1.0f,  1.0f, -1.0f,
				-1.0f, -1.0f, -1.0f,
				 1.0f, -1.0f, -1.0f,
				 1.0f, -1.0f, -1.0f,
				 1.0f,  1.0f, -1.0f,
				-1.0f,  1.0f, -1.0f,

				-1.0f, -1.0f,  1.0f,
				-1.0f, -1.0f, -1.0f,
				-1.0f,  1.0f, -1.0f,
				-1.0f,  1.0f, -1.0f,
				-1.0f,  1.0f,  1.0f,
				-1.0f, -1.0f,  1.0f,

				 1.0f, -1.0f, -1.0f,
				 1.0f, -1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f, -1.0f,
				 1.0f, -1.0f, -1.0f,

				-1.0f, -1.0f,  1.0f,
				-1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f, -1.0f,  1.0f,
				-1.0f, -1.0f,  1.0f,

				-1.0f,  1.0f, -1.0f,
				 1.0f,  1.0f, -1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				-1.0f,  1.0f,  1.0f,
				-1.0f,  1.0f, -1.0f,

				-1.0f, -1.0f, -1.0f,
				-1.0f, -1.0f,  1.0f,
				 1.0f, -1.0f, -1.0f,
				 1.0f, -1.0f, -1.0f,
				-1.0f, -1.0f,  1.0f,
				 1.0f, -1.0f,  1.0f
			};

			GraphicsPipelineCreateInfo DynamicPipelineCI = {};
			{
				DynamicPipelineCI.VertexInputInfos = { VertexInputInfo(sizeof(SkyBoxData), layout) };
				DynamicPipelineCI.PipelineName = "Skybox_Pipiline";
				DynamicPipelineCI.ShaderCreateInfo = shaderCI;
				DynamicPipelineCI.TargetFramebuffers = { &m_Framebuffer };
			}

			auto result = m_GraphicsPipeline.Create(&DynamicPipelineCI);
			assert(result == PipelineCreateResult::SUCCESS);
			Ref<VertexBuffer> skyBoxFB = std::make_shared<VertexBuffer>();
			VertexBuffer::Create(skyBoxFB.get(), skyboxVertices, sizeof(skyboxVertices));
			m_GraphicsPipeline.SetVertexBuffers({ skyBoxFB });
		}
		
	}

	void EnvironmentMap::GenerateStatic(CubeMap* cubeMap)
	{
		Free();
		m_IsDynamic = false;
		m_CubeMap = cubeMap;
		VulkanPBR* instance = VulkanPBR::GetSingleton();
		instance->GeneratePBRCubeMaps(m_CubeMap);
	}

	void EnvironmentMap::GenerateDynamic(const glm::mat4& cameraProj)
	{
		Free();
		m_IsDynamic = true;
		m_CubeMap = new CubeMap();
		CubeMap::CreateEmpty(m_CubeMap, 1024, 1024);

		struct push_constant
		{
			glm::mat4 view;
			glm::mat4 proj;
		} pc;

		CommandBufferStorage cmdStorage = {};
		VulkanCommandBuffer::CreateCommandBuffer(&cmdStorage);
		{
			UpdateDescriptors();
			m_GraphicsPipeline.SetCommandBuffer(cmdStorage.Buffer);
			pc.proj = cameraProj == glm::mat4(0.0f) ? glm::perspective(glm::radians(75.0f), 1.0f, 0.1f, 1000.0f): cameraProj;

			for (uint32_t face = 0; face < 6; face++)
			{
				m_GraphicsPipeline.BeginRenderPass();

				glm::mat4 viewMatrix = glm::mat4(1.0f);
				switch (face)
				{
				case 0: // POSITIVE_X
					viewMatrix = glm::rotate(viewMatrix, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
					viewMatrix = glm::rotate(viewMatrix, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
					break;
				case 1:	// NEGATIVE_X
					viewMatrix = glm::rotate(viewMatrix, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
					viewMatrix = glm::rotate(viewMatrix, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
					break;
				case 2:	// POSITIVE_Y
					viewMatrix = glm::rotate(viewMatrix, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
					break;
				case 3:	// NEGATIVE_Y
					viewMatrix = glm::rotate(viewMatrix, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
					break;
				case 4:	// POSITIVE_Z
					viewMatrix = glm::rotate(viewMatrix, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
					break;
				case 5:	// NEGATIVE_Z
					viewMatrix = glm::rotate(viewMatrix, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
					break;
				}
				pc.view = viewMatrix;

				m_GraphicsPipeline.SubmitPushConstant(ShaderType::Vertex, sizeof(push_constant), &pc);
				m_GraphicsPipeline.Draw(36);
				m_GraphicsPipeline.EndRenderPass();

				// Copy commands
				{
					auto cube_image = m_CubeMap->GetTexture()->GetVulkanTexture()->GetVkImage();
					auto fb_image = m_Framebuffer.GetVulkanFramebuffer().GetAttachment()->AttachmentVkInfo.image;

					// Make sure color writes to the framebuffer are finished before using it as transfer source
					VulkanTexture::SetImageLayout(
						cmdStorage.Buffer,
						fb_image,
						VK_IMAGE_ASPECT_COLOR_BIT,
						VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
						VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

					VkImageSubresourceRange cubeFaceSubresourceRange = {};
					cubeFaceSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					cubeFaceSubresourceRange.baseMipLevel = 0;
					cubeFaceSubresourceRange.levelCount = 1;
					cubeFaceSubresourceRange.baseArrayLayer = face;
					cubeFaceSubresourceRange.layerCount = 1;

					// Change image layout of one cubemap face to transfer destination
				    VulkanTexture::SetImageLayout(
						cmdStorage.Buffer,
						cube_image,
						VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
						VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						cubeFaceSubresourceRange);

					// Copy region for transfer from framebuffer to cube face
					VkImageCopy copyRegion = {};

					copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					copyRegion.srcSubresource.baseArrayLayer = 0;
					copyRegion.srcSubresource.mipLevel = 0;
					copyRegion.srcSubresource.layerCount = 1;
					copyRegion.srcOffset = { 0, 0, 0 };

					copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					copyRegion.dstSubresource.baseArrayLayer = face;
					copyRegion.dstSubresource.mipLevel = 0;
					copyRegion.dstSubresource.layerCount = 1;
					copyRegion.dstOffset = { 0, 0, 0 };

					copyRegion.extent.width = m_CubeMap->GetTexture()->GetWidth();
					copyRegion.extent.height = m_CubeMap->GetTexture()->GetHeight();
					copyRegion.extent.depth = 1;

					// Put image copy into command buffer
					vkCmdCopyImage(
						cmdStorage.Buffer,
						fb_image,
						VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						cube_image,
						VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						1,
						&copyRegion);

					// Transform framebuffer color attachment back
					VulkanTexture::SetImageLayout(
						cmdStorage.Buffer,
						fb_image,
						VK_IMAGE_ASPECT_COLOR_BIT,
						VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

					// Change image layout of copied face to shader read
					VulkanTexture::SetImageLayout(
						cmdStorage.Buffer,
						cube_image,
						VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
						cubeFaceSubresourceRange);
				}

			}
		}
		VulkanCommandBuffer::ExecuteCommandBuffer(&cmdStorage);
	}

	void EnvironmentMap::UpdateDescriptors()
	{
		m_GraphicsPipeline.SubmitBuffer(512, sizeof(DynamicSkyProperties), &m_UBO);
	}

	CubeMap* EnvironmentMap::GetCubeMap() const
	{
		return m_CubeMap;
	}

	DynamicSkyProperties& EnvironmentMap::GetDynamicSkyProperties()
	{
		return m_UBO;
	}

	void EnvironmentMap::Free()
	{
		if (m_CubeMap != nullptr)
		{
			delete m_CubeMap;
			m_CubeMap = nullptr;
		}
	}

	bool EnvironmentMap::IsReady() const
	{
		return m_CubeMap->GetTexture()->IsInitialized();
	}

	bool EnvironmentMap::IsDynamic() const
	{
		return m_IsDynamic;
	}
}