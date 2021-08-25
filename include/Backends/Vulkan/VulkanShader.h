#pragma once
#ifndef FROSTIUM_OPENGL_IMPL

#include "Backends/Vulkan/Vulkan.h"
#include "Backends/Vulkan/VulkanDescriptor.h"

#include <string>
#include <unordered_map>
#include <vector>

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	struct ReflectionData;
	struct GraphicsPipelineShaderCreateInfo;
	enum class ShaderType : int;

	class VulkanShader
	{
	public:

		VulkanShader();
		~VulkanShader();
														         
		bool                                                     Init(std::unordered_map<ShaderType, std::vector<uint32_t>>& binary, ReflectionData* reflectData, GraphicsPipelineShaderCreateInfo* createInfo);
		void                                                     Clean();
														         
		std::vector<VkPipelineShaderStageCreateInfo>&            GetVkPipelineShaderStages();
		static VkShaderStageFlagBits                             GetVkShaderStage(ShaderType type);
														         
	private:											         
		void                                                     DeleteShaderModules();

	private:
		ReflectionData*                                          m_ReflectionData = nullptr;
		GraphicsPipelineShaderCreateInfo*                        m_CreateInfo = nullptr;
		std::unordered_map<ShaderType, VkShaderModule>           m_ShaderModules;
		std::vector<VkPushConstantRange>                         m_VkPushConstantRanges;
		std::vector<VkPipelineShaderStageCreateInfo>             m_PipelineShaderStages;

	private:

		friend class VulkanPipeline;
		friend class VulkanPBR;
		friend class VulkanDescriptor;
		friend class GraphicsPipeline;
	};
}
#endif