#include "stdafx.h"
#ifndef OPENGL_IMPL
#include "Backends/Vulkan/VulkanShader.h"
#include "Backends/Vulkan/VulkanContext.h"
#include "Backends/Vulkan/VulkanUtils.h"
#include "Backends/Vulkan/VulkanRaytracingPipeline.h"

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
                {
                    VkShaderModuleCreateInfo shaderModuleCI = {};
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

                m_VkPipelineShaderStages.emplace_back(pipelineShaderStageCI);
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

    bool VulkanShader::BuildRT(ShaderCreateInfo* info)
    {
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
        m_VkPipelineShaderStages.clear();
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

    void VulkanShader::CreateShaderBindingTable(VulkanRaytracingPipeline* pipeline)
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        const uint32_t handleSize = device.rayTracingPipelineProperties.shaderGroupHandleSize;
        const uint32_t handleSizeAligned = VulkanUtils::GetAlignedSize(device.rayTracingPipelineProperties.shaderGroupHandleSize, device.rayTracingPipelineProperties.shaderGroupHandleAlignment);
        const uint32_t groupCount = static_cast<uint32_t>(m_ShaderGroupsRT.size());
        const uint32_t sbtSize = groupCount * handleSizeAligned;

        std::vector<uint8_t> shaderHandleStorage(sbtSize);
        VK_CHECK_RESULT(device.vkGetRayTracingShaderGroupHandlesKHR(device.GetLogicalDevice(), pipeline->GetVkPipeline(), 0, groupCount, sbtSize, shaderHandleStorage.data()));

        auto& rayGen = m_BindingTables[ShaderType::RayGen];
        auto& rayHit = m_BindingTables[ShaderType::RayCloseHit]; // temp
        auto& rayMiss = m_BindingTables[ShaderType::RayMiss];

        rayGen.CreateBuffer(handleSize, VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
        rayHit.CreateBuffer(handleSize, VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
        rayMiss.CreateBuffer(handleSize, VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);

        void* rayGenSrc = rayGen.MapMemory();
        void* rayHitSrc = rayHit.MapMemory();
        void* rayMissSrc = rayMiss.MapMemory();

        memcpy(rayGenSrc, shaderHandleStorage.data(), handleSize);
        memcpy(rayMissSrc, shaderHandleStorage.data() + handleSizeAligned, handleSize);
        memcpy(rayHitSrc, shaderHandleStorage.data() + handleSizeAligned * 2, handleSize);

        rayGen.UnMapMemory();
        rayHit.UnMapMemory();
        rayMiss.UnMapMemory();
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

        if ((type & ShaderType::RayGen) == ShaderType::RayGen)
        {
            flags |= VkShaderStageFlagBits::VK_SHADER_STAGE_RAYGEN_BIT_KHR;
        }

        if ((type & ShaderType::RayAnyHit) == ShaderType::RayAnyHit)
        {
            flags |= VkShaderStageFlagBits::VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
        }

        if ((type & ShaderType::RayCloseHit) == ShaderType::RayCloseHit)
        {
            flags |= VkShaderStageFlagBits::VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
        }

        if ((type & ShaderType::RayMiss) == ShaderType::RayMiss)
        {
            flags |= VkShaderStageFlagBits::VK_SHADER_STAGE_MISS_BIT_KHR;
        }

        return (VkShaderStageFlagBits)flags;
    }

    std::vector<VkPipelineShaderStageCreateInfo>& VulkanShader::GetVkPipelineShaderStages()
    {
        return m_VkPipelineShaderStages;
    }
}
#endif