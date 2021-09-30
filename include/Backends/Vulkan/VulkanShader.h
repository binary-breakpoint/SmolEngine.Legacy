#pragma once
#ifndef OPENGL_IMPL

#include "Backends/Vulkan/Vulkan.h"
#include "Backends/Vulkan/VulkanDescriptor.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace SmolEngine
{
	struct ReflectionData;
	struct ShaderCreateInfo;
	enum class ShaderType : int;

	class VulkanShader
	{
	public:										         
		bool                                                     Init(std::unordered_map<ShaderType, std::vector<uint32_t>>& binary, ReflectionData* reflectData, ShaderCreateInfo* createInfo);
		void                                                     Clean();									         
		std::vector<VkPipelineShaderStageCreateInfo>&            GetVkPipelineShaderStages();
		static VkShaderStageFlagBits                             GetVkShaderStage(ShaderType type);
														         
	private:											         
		void                                                     DeleteShaderModules();

	private:
		ReflectionData*                                          m_ReflectionData = nullptr;
		ShaderCreateInfo*                                        m_CreateInfo = nullptr;
		std::unordered_map<ShaderType, VkShaderModule>           m_ShaderModules;
		std::vector<VkPushConstantRange>                         m_VkPushConstantRanges;
		std::vector<VkPipelineShaderStageCreateInfo>             m_PipelineShaderStages;

	private:
		friend class VulkanPipeline;
		friend class VulkanPBR;
		friend class VulkanDescriptor;
		friend class GraphicsPipeline;
		friend class VulkanComputePipeline;
	};
}
#endif