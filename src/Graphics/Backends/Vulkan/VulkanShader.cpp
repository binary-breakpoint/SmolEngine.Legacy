#include "stdafx.h"
#ifndef OPENGL_IMPL
#include "Graphics/Backends/Vulkan/VulkanShader.h"
#include "Graphics/Backends/Vulkan/VulkanContext.h"
#include "Graphics/Backends/Vulkan/VulkanUtils.h"
#include "Graphics/Backends/Vulkan/VulkanContext.h"

#include "Graphics/Tools/Utils.h"

namespace SmolEngine
{
    bool VulkanShader::Build(ShaderCreateInfo* info)
    {
        if (!BuildBase(info)) { return false; }

#ifdef  SMOLENGINE_DEBUG
        for (auto& [type, path_raw] : info->Stages)
        {
            std::string path = Utils::GetCachedPath(path_raw, CachedPathType::Shader);
            if (Utils::IsPathValid(path))
                VulkanContext::GetCrashTracker().m_tracker.AddShaderBinary(path.c_str());
        }
#endif
        std::unordered_map<ShaderType, uint32_t> shaderID;

        // Shader Modules
        for (auto& [type, data] : m_Binary)
        {
            VkShaderStageFlagBits vkStage = GetVkShaderStage(type);
            VkShaderModule shaderModule = nullptr;
            {
                VkShaderModuleCreateInfo shaderModuleCI = {};
                shaderModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
                shaderModuleCI.codeSize = data.size() * sizeof(uint32_t);
                shaderModuleCI.pCode = data.data();

                VK_CHECK_RESULT(vkCreateShaderModule(VulkanContext::GetDevice().GetLogicalDevice(), &shaderModuleCI, nullptr, &shaderModule));
            }

            VkPipelineShaderStageCreateInfo pipelineShaderStageCI{};
            {
                pipelineShaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                pipelineShaderStageCI.stage = vkStage;
                pipelineShaderStageCI.pName = "main";
                pipelineShaderStageCI.module = shaderModule;

                assert(pipelineShaderStageCI.module != VK_NULL_HANDLE);
            }

            m_VkPipelineShaderStages.emplace_back(pipelineShaderStageCI);
            m_ShaderModules[type] = shaderModule;
            shaderID[type] = static_cast<uint32_t>(m_VkPipelineShaderStages.size());
        }

        m_Binary.clear();

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

        if (m_RTPipeline)
        {
            // Ray generation group
            {
                VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
                shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
                shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
                shaderGroup.generalShader = shaderID[ShaderType::RayGen] - 1;
                shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
                shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
                shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;

                m_ShaderGroupsRT.push_back(shaderGroup);
            }

            // Ray miss group
            {
                VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
                shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
                shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
                shaderGroup.generalShader = shaderID[ShaderType::RayMiss] - 1;
                shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
                shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
                shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;

                m_ShaderGroupsRT.push_back(shaderGroup);
            }

            // Ray closest hit group
            {
                VkRayTracingShaderGroupCreateInfoKHR shaderGroup{ VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR };
                shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
                shaderGroup.generalShader = VK_SHADER_UNUSED_KHR;
                shaderGroup.closestHitShader = shaderID[ShaderType::RayCloseHit] - 1;
                shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
                shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;

                m_ShaderGroupsRT.push_back(shaderGroup);
            }
        }

        return true;
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

    void VulkanShader::CreateShaderBindingTable(VkPipeline vkPipeline)
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        std::vector<uint32_t> rgen_index{ 0 };
        std::vector<uint32_t> miss_index{ 1, };
        std::vector<uint32_t> hit_index{ 2 };

        const uint32_t handle_size = device.rayTracingPipelineProperties.shaderGroupHandleSize;
        const uint32_t handle_alignment = device.rayTracingPipelineProperties.shaderGroupHandleAlignment;
        const uint32_t handle_size_aligned = VulkanUtils::GetAlignedSize(handle_size, handle_alignment);

        auto& rayGen = m_BindingTables[ShaderType::RayGen];
        auto& rayMiss = m_BindingTables[ShaderType::RayMiss];
        auto& rayHit = m_BindingTables[ShaderType::RayCloseHit];

        const VkBufferUsageFlags sbt_buffer_usage_flags = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

        rayGen.CreateBuffer(handle_size_aligned * rgen_index.size(), sbt_buffer_usage_flags);
        rayMiss.CreateBuffer(handle_size_aligned * miss_index.size(), sbt_buffer_usage_flags);
        rayHit.CreateBuffer(handle_size_aligned * hit_index.size(), sbt_buffer_usage_flags);

        // Copy the pipeline's shader handles into a host buffer
        const auto           group_count = static_cast<uint32_t>(rgen_index.size() + miss_index.size() + hit_index.size());
        const auto           sbt_size = group_count * handle_size_aligned;
        std::vector<uint8_t> shader_handle_storage(sbt_size);
        VK_CHECK_RESULT(device.vkGetRayTracingShaderGroupHandlesKHR(device.GetLogicalDevice(), vkPipeline, 0, group_count, sbt_size, shader_handle_storage.data()));

        // Write the handles in the SBT buffer
        auto copyHandles = [&](auto& buffer, std::vector<uint32_t>& indices, uint32_t stride) 
        {
            auto* pBuffer = static_cast<uint8_t*>(buffer.MapMemory());

            for (uint32_t index = 0; index < static_cast<uint32_t>(indices.size()); index++)
            {
                auto* pStart = pBuffer;
                // Copy the handle
                memcpy(pBuffer, shader_handle_storage.data() + (indices[index] * handle_size), handle_size);
                pBuffer = pStart + stride;        // Jumping to next group
            }

            buffer.UnMapMemory();
        };

        copyHandles(rayGen, rgen_index, handle_size_aligned);
        copyHandles(rayMiss, miss_index, handle_size_aligned);
        copyHandles(rayHit, hit_index, handle_size_aligned);
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