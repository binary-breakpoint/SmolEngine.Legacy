#pragma once
#ifndef OPENGL_IMPL

#include "Backends/Vulkan/Vulkan.h"
#include "Backends/Vulkan/VulkanDescriptor.h"
#include "Primitives/BufferElement.h"
#include "Primitives/GraphicsPipeline.h"

namespace SmolEngine
{
	class VulkanPipeline: public GraphicsPipeline
	{
	public:
		~VulkanPipeline();

		// Interface
		bool                                            Build(GraphicsPipelineCreateInfo* pipelineInfo) override;
		void                                            ClearColors(const glm::vec4& clearColors = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f)) override;
		void                                            BeginRenderPass(bool flip = false) override;
		void                                            EndRenderPass() override;
		void                                            BeginCommandBuffer(bool isMainCmdBufferInUse = false) override;
		void                                            EndCommandBuffer()  override;
		void                                            Destroy() override;
		void                                            Reload() override;

		void                                            DrawIndexed(uint32_t vbIndex = 0, uint32_t ibIndex = 0) override;
		void                                            DrawIndexed(VertexBuffer* vb, IndexBuffer* ib) override;
		void                                            Draw(VertexBuffer* vb, uint32_t vertextCount) override;
		void                                            Draw(uint32_t vertextCount, uint32_t vertexBufferIndex = 0) override;
		void                                            DrawMeshIndexed(Mesh* mesh, uint32_t instances = 1) override;
		void                                            DrawMesh(Mesh* mesh, uint32_t instances = 1) override;
								                        
		bool                                            SubmitBuffer(uint32_t bindingPoint, size_t size, const void* data, uint32_t offset = 0) override;
		void                                            SubmitPushConstant(ShaderType shaderStage, size_t size, const void* data) override;
								                        
		bool                                            UpdateSamplers(const std::vector<Ref<Texture>>& textures, uint32_t bindingPoint, bool storageImage = false) override;
		bool                                            UpdateSampler(Ref<Texture>& tetxure, uint32_t bindingPoint, bool storageImage = false) override;
		bool                                            UpdateSampler(Framebuffer* framebuffer, uint32_t bindingPoint, uint32_t attachmentIndex = 0) override;
		bool                                            UpdateSampler(Framebuffer* framebuffer, uint32_t bindingPoint, const std::string& attachmentName) override;
		bool                                            UpdateCubeMap(Ref<Texture>& cubeMap, uint32_t bindingPoint) override;
								                        
		void                                            BindPipeline() override;
		void                                            BindDescriptors() override;
		void                                            BindIndexBuffer(uint32_t index = 0) override;
		void                                            BindVertexBuffer(uint32_t index = 0) override;
		// Main
		bool                                            CreatePipeline(DrawMode mode);
		const VkPipeline&                               GetVkPipeline(DrawMode mode);
		const VkPipelineLayout&                         GetVkPipelineLayot() const;
		VkCommandBuffer                                 GetCommandBuffer();
		const VkDescriptorSet                           GetVkDescriptorSets(uint32_t setIndex = 0) const;
		void                                            SetCommandBuffer(VkCommandBuffer cmd);
		bool                                            SaveCache(const std::string& fileName, DrawMode mode);
		bool                                            CreateOrLoadCached(const std::string& fileName, DrawMode mode);
		void                                            UpdateImageDescriptor(uint32_t bindingPoint, const VkDescriptorImageInfo& imageInfo);
		// Helpers						                
		static void                                     BuildDescriptors(Ref<Shader>& shader, uint32_t descriptorSets, 
			                                            std::vector<VulkanDescriptor>& outDescriptors, VkDescriptorPool& pool);
	private:							                
		bool                                            IsBlendEnableEnabled();
		VkFormat                                        GetVkInputFormat(DataTypes type);
		VkPrimitiveTopology                             GetVkTopology(DrawMode mode);
		VkCullModeFlags                                 GetVkCullMode(CullMode mode);
		VkPolygonMode                                   GetVkPolygonMode(PolygonMode mode);
		VkBlendFactor                                   GetVkBlendFactor(BlendFactor factor);
		VkBlendOp                                       GetVkBlendOp(BlendOp op);

	private:
		bool                                            m_IsMainCmdBufferInUse = false;
		VkPipelineLayout                                m_PipelineLayout = VK_NULL_HANDLE;
		VkRenderPass                                    m_TargetRenderPass = nullptr;
		VkDevice                                        m_Device = nullptr;
		VkCommandBuffer                                 m_CommandBuffer = nullptr;
		VkDescriptorPool                                m_DescriptorPool = nullptr;
		CommandBufferStorage                            m_CmdStorage{};
		std::vector<VulkanDescriptor>                   m_Descriptors;
		std::vector<VkDescriptorSetLayout>              m_SetLayout;
		std::unordered_map<DrawMode, VkPipelineCache>   m_PipelineCaches;
		std::unordered_map<DrawMode, VkPipeline>        m_Pipelines;
		std::string                                     m_FilePath = "";

	private:

		friend class GraphicsPipeline;
	};
}
#endif