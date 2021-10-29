#pragma once
#ifndef OPENGL_IMPL
#include "Backends/Vulkan/Vulkan.h"
#include "Backends/Vulkan/VulkanDescriptor.h"
#include "Backends/Vulkan/VulkanBuffer.h"

#include "Primitives/Shader.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace SmolEngine
{
	class VulkanRaytracingPipeline;

	class VulkanShader: public Shader
	{
	public:		
		bool                                                     Build(ShaderCreateInfo* info) override;
		bool                                                     BuildRT(ShaderCreateInfo* info);
		bool                                                     Realod() override;
		void                                                     Free() override;
							         
		std::vector<VkPipelineShaderStageCreateInfo>&            GetVkPipelineShaderStages();									        									         
		static VkShaderStageFlagBits                             GetVkShaderStage(ShaderType type);
		void                                                     DeleteShaderModules();

	private:
		void                                                     CreateShaderBindingTable(VulkanRaytracingPipeline* pipeline);

	private:
		std::vector<VkPushConstantRange>                         m_VkPushConstantRanges;
		std::vector<VkPipelineShaderStageCreateInfo>             m_VkPipelineShaderStages;
		std::vector<VkRayTracingShaderGroupCreateInfoKHR>        m_ShaderGroupsRT{};
		std::unordered_map<ShaderType, VkShaderModule>           m_ShaderModules;
		std::unordered_map<ShaderType, VulkanBuffer>             m_BindingTables;

		friend class VulkanPipeline;
		friend class VulkanPBRLoader;
		friend class VulkanDescriptor;
		friend class GraphicsPipeline;
		friend class VulkanComputePipeline;
	};
}
#endif