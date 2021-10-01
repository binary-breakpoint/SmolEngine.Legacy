#include "stdafx.h"
#ifndef OPENGL_IMPL
#include "Common/Common.h"
#include "Backends/Vulkan/VulkanShader.h"
#include "Backends/Vulkan/VulkanContext.h"

#include "Primitives/Shader.h"

#include "Tools/Utils.h"
#include <shaderc/shaderc.hpp>

namespace SmolEngine
{
    bool VulkanShader::Build(ShaderCreateInfo* info)
    {
        if (BuildBase(info))
        {
            // Shader Modules
            for (auto& [type, data] : m_Binary)
            {
                VkShaderModule shaderModule = nullptr;
                VkShaderModuleCreateInfo shaderModuleCI = {};
                {
                    shaderModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
                    shaderModuleCI.codeSize = data.size() * sizeof(uint32_t);
                    shaderModuleCI.pCode = data.data();

                    VK_CHECK_RESULT(vkCreateShaderModule(VulkanContext::GetDevice().GetLogicalDevice(), &shaderModuleCI, nullptr, &shaderModule));
                }

                VkPipelineShaderStageCreateInfo pipelineShaderStageCI = {};
                {
                    pipelineShaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                    pipelineShaderStageCI.stage = GetVkShaderStage(type);
                    pipelineShaderStageCI.pName = "main";
                    pipelineShaderStageCI.module = shaderModule;
                    assert(pipelineShaderStageCI.module != VK_NULL_HANDLE);
                }

                m_PipelineShaderStages.emplace_back(pipelineShaderStageCI);
                m_ShaderModules[type] = shaderModule;
            }

            // Push Constant
            if (m_ReflectData.PushConstant.Size > 0)
            {
                VkPushConstantRange range = {};
                {
                    range.offset = m_ReflectData.PushConstant.Offset;
                    range.size = m_ReflectData.PushConstant.Size;
                    range.stageFlags = GetVkShaderStage(m_ReflectData.PushConstant.Stage);
                }

                m_VkPushConstantRanges.emplace_back(range);
            }

            m_Binary.clear();
            return true;
        }

        return false;
    }

    bool VulkanShader::Realod()
    {
        Free();
        return Build(&m_CreateInfo);
    }

    void VulkanShader::Free()
    {
        DeleteShaderModules();
        m_VkPushConstantRanges.clear();
        m_PipelineShaderStages.clear();
    }

    void VulkanShader::DeleteShaderModules()
    {
        const auto& device = VulkanContext::GetDevice().GetLogicalDevice();
        for (auto& [key, module] : m_ShaderModules)
        {
            vkDestroyShaderModule(device, module, nullptr);
        }

        m_ShaderModules.clear();
        m_ReflectData.Clean();
    }

    VkShaderStageFlagBits VulkanShader::GetVkShaderStage(ShaderType type)
    {
        VkShaderStageFlags flags = 0;

        if ((type & ShaderType::Vertex) == ShaderType::Vertex)
        {
            flags |= VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
        }

        if ((type & ShaderType::Fragment) == ShaderType::Fragment)
        {
            flags |= VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
        }

        if ((type & ShaderType::Compute) == ShaderType::Compute)
        {
            flags |= VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT;
        }
        if ((type & ShaderType::Geometry) == ShaderType::Geometry)
        {
            flags |= VkShaderStageFlagBits::VK_SHADER_STAGE_GEOMETRY_BIT;
        }

        return (VkShaderStageFlagBits)flags;
    }

    std::vector<VkPipelineShaderStageCreateInfo>& VulkanShader::GetVkPipelineShaderStages()
    {
        return m_PipelineShaderStages;
    }
}
#endif