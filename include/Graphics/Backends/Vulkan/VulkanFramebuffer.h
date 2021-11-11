#pragma once
#ifndef OPENGL_IMPL
#include "Graphics/Backends/Vulkan/Vulkan.h"
#include "Graphics/Primitives/Framebuffer.h"

#include <vector>
#include <unordered_map>

namespace SmolEngine
{
	struct VulkanFramebufferAttachment
	{
		VkImage         image;
		VmaAllocation   alloc;
		VkImageView     view;
	};

	struct Attachment
	{
		VulkanFramebufferAttachment   AttachmentVkInfo;
		VkDescriptorImageInfo         ImageInfo;
		void*                         ImGuiID = nullptr;
	};

	class VulkanFramebuffer: public Framebuffer
	{
	public:
		~VulkanFramebuffer();

		bool                                             Build(FramebufferSpecification* info) override;
		void                                             Free() override;
		void                                             OnResize(uint32_t width, uint32_t height) override;
		void*                                            GetImGuiTextureID(uint32_t index = 0) override;
		void                                             SetClearColor(const glm::vec4& color) override;

		// Getters
		const std::vector<VkClearAttachment>&            GetClearAttachments() const;
		const std::vector<VkClearValue>&                 GetClearValues() const;
		const VkFramebuffer                              GetCurrentVkFramebuffer() const;
		const VkFramebuffer                              GetVkFramebuffer(uint32_t index) const;
		static VkFormat                                  GetAttachmentFormat(AttachmentFormat format);
		VkRenderPass                                     GetRenderPass() const;
		VkSampleCountFlagBits                            GetMSAASamples() const;
		const uint32_t                                   GetAttachmentCount() const;
		Attachment*                                      GetAttachment(uint32_t index = 0);
		Attachment*                                      GetAttachment(const std::string& name);
		Attachment*                                      GetDethAttachment();

	private:
		bool                                             Create(uint32_t width, uint32_t height);
		bool                                             CreateShadow(uint32_t width, uint32_t height);
		bool                                             CreateCopyFramebuffer(uint32_t width, uint32_t height);
		void                                             CreateSampler(VkFilter filer = VK_FILTER_NEAREST);
		void                                             FreeAttachment(Attachment& framebuffer);
		bool                                             IsUseMSAA();
		VkBool32                                         IsFormatIsFilterable(VkPhysicalDevice physicalDevice, VkFormat format, VkImageTiling tiling);
		VkSampleCountFlagBits                            GetVkMSAASamples(MSAASamples samples);
		void                                             AddAttachment(uint32_t width, uint32_t height, VkSampleCountFlagBits samples, VkImageUsageFlags imageUsage,
			                                             VkFormat format, VkImage& image, VkImageView& imageView, VmaAllocation& mem,
			                                             VkImageAspectFlags imageAspect = VK_IMAGE_ASPECT_COLOR_BIT);

	private:
		VkDevice                                         m_Device = nullptr;
		VkSampler                                        m_Sampler = nullptr;
		VkRenderPass                                     m_RenderPass = nullptr;
		VkSampleCountFlagBits                            m_MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		VkFormat                                         m_ColorFormat;
		VkFormat                                         m_DepthFormat;
		Attachment                                       m_DepthAttachment = {};
		std::vector<VkFramebuffer>                       m_VkFrameBuffers;
		std::vector<VkClearAttachment>                   m_ClearAttachments;
		std::vector<VkClearValue>                        m_ClearValues;
		std::vector<Attachment>                          m_Attachments;
		std::vector<Attachment>                          m_ResolveAttachments;
		std::unordered_map<std::string, uint32_t>        m_AttachmentsMap;

	private:

		friend class GraphicsPipeline;
		friend class VulkanSwapchain;
	};
}
#endif