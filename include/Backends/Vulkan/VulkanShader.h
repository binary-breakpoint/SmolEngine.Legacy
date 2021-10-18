#pragma once
#ifndef OPENGL_IMPL

#include "Backends/Vulkan/Vulkan.h"
#include "Backends/Vulkan/VulkanDescriptor.h"
#include "Primitives/Shader.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace SmolEngine
{
	class VulkanShader: public Shader
	{
	public:							
		bool                                                     Build(ShaderCreateInfo* info) override;
		bool                                                     Realod() override;
		void                                                     Free() override;
							         
		std::vector<VkPipelineShaderStageCreateInfo>&            GetVkPipelineShaderStages();									        									         
		void                                                     DeleteShaderModules();
		static VkShaderStageFlagBits                             GetVkShaderStage(ShaderType type);

	private:
		std::vector<VkPushConstantRange>                         m_VkPushConstantRanges;
		std::vector<VkPipelineShaderStageCreateInfo>             m_PipelineShaderStages;
		std::unordered_map<ShaderType, VkShaderModule>           m_ShaderModules;

		friend class VulkanPipeline;
		friend class VulkanPBRLoader;
		friend class VulkanDescriptor;
		friend class GraphicsPipeline;
		friend class VulkanComputePipeline;
	};
}
#endif