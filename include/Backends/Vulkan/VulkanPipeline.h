#pragma once
#ifndef FROSTIUM_OPENGL_IMPL

#include "Backends/Vulkan/Vulkan.h"
#include "Backends/Vulkan/VulkanDescriptor.h"
#include "Primitives/BufferElement.h"

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	class VulkanShader;
	class VulkanTexture;
	class VulkanShader;

	struct GraphicsPipelineCreateInfo;

	enum class DrawMode : uint16_t;
	enum class CullMode : uint16_t;
	enum class BlendFactor : uint16_t;
	enum class BlendOp : uint16_t;
	enum class PolygonMode : uint16_t;

	class VulkanPipeline
	{
	public:
		bool                                            Invalidate(GraphicsPipelineCreateInfo* pipelineSpec, VulkanShader* shader);
		bool                                            CreatePipeline(DrawMode mode);
		bool                                            ReCreate();
		void                                            Destroy();

		const VkPipeline&                               GetVkPipeline(DrawMode mode);
		const VkPipelineLayout&                         GetVkPipelineLayot() const;
		const VkDescriptorSet                           GetVkDescriptorSets(uint32_t setIndex = 0) const;
		bool                                            SaveCache(const std::string& fileName, DrawMode mode);
		bool                                            CreateOrLoadCached(const std::string& fileName, DrawMode mode);
										                
		// Helpers						                
		static void                                     BuildDescriptors(VulkanShader* shader, uint32_t descriptorSets, 
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
		VkPipelineLayout                                m_PipelineLayout = VK_NULL_HANDLE;
		VkRenderPass                                    m_TargetRenderPass = nullptr;
		VkDevice                                        m_Device = nullptr;
		VulkanShader*                                   m_Shader = nullptr;

		VkDescriptorPool                                m_DescriptorPool = nullptr;
		GraphicsPipelineCreateInfo*                     m_PipelineSpecification = nullptr;

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