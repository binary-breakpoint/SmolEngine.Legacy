#include "stdafx.h"
#ifndef OPENGL_IMPL
#include "Backends/Vulkan/VulkanRaytracingPipeline.h"
#include "Backends/Vulkan/VulkanContext.h"
#include "Backends/Vulkan/VulkanShader.h"
#include "Backends/Vulkan/VulkanPipeline.h"
#include "Backends/Vulkan/VulkanFramebuffer.h"
#include "Backends/Vulkan/VulkanUtils.h"
#include "Tools/Utils.h"

namespace SmolEngine
{
	void VulkanRaytracingPipeline::Dispatch(uint32_t width, uint32_t height)
	{
		const VulkanDevice& device = VulkanContext::GetDevice();
		const VkDescriptorSet& descriptorSet = m_Descriptors[m_DescriptorIndex].GetDescriptorSets();
		auto& shaderBindingTable = m_Shader->Cast<VulkanShader>()->m_BindingTables;

		const uint32_t handleSizeAligned = VulkanUtils::GetAlignedSize(device.rayTracingPipelineProperties.shaderGroupHandleSize,
			device.rayTracingPipelineProperties.shaderGroupHandleAlignment);

		VkStridedDeviceAddressRegionKHR raygenShaderSbtEntry{};
		raygenShaderSbtEntry.deviceAddress = VulkanUtils::GetBufferDeviceAddress(shaderBindingTable[ShaderType::RayGen].GetBuffer());
		raygenShaderSbtEntry.stride = handleSizeAligned;
		raygenShaderSbtEntry.size = handleSizeAligned;

		VkStridedDeviceAddressRegionKHR missShaderSbtEntry{};
		missShaderSbtEntry.deviceAddress = VulkanUtils::GetBufferDeviceAddress(shaderBindingTable[ShaderType::RayMiss].GetBuffer());
		missShaderSbtEntry.stride = handleSizeAligned;
		missShaderSbtEntry.size = handleSizeAligned;

		VkStridedDeviceAddressRegionKHR hitShaderSbtEntry{};
		hitShaderSbtEntry.deviceAddress = VulkanUtils::GetBufferDeviceAddress(shaderBindingTable[ShaderType::RayCloseHit].GetBuffer());
		hitShaderSbtEntry.stride = handleSizeAligned;
		hitShaderSbtEntry.size = handleSizeAligned;

		VkStridedDeviceAddressRegionKHR callableShaderSbtEntry{};

		{
			vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_Pipeline);
			vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_PipelineLayout, 0, 1, &descriptorSet, 0, 0);

			device.vkCmdTraceRaysKHR(
				m_CommandBuffer,
				&raygenShaderSbtEntry,
				&missShaderSbtEntry,
				&hitShaderSbtEntry,
				&callableShaderSbtEntry,
				width,
				height,
				1);
		}
		m_CommandBuffer = nullptr;
	}

	void VulkanRaytracingPipeline::SetCommandBuffer(void* cmdStorage)
	{
		const CommandBufferStorage* storage = (CommandBufferStorage*)cmdStorage;
		m_CommandBuffer = cmdStorage != nullptr ? storage->Buffer : VulkanContext::GetCurrentVkCmdBuffer();
	}

	void VulkanRaytracingPipeline::Free()
	{
		m_BottomLevelAS.clear();
		m_TopLevelAS.Free();

		{
			VkDevice device = VulkanContext::GetDevice().GetLogicalDevice();

			vkDestroyPipeline(device, m_Pipeline, nullptr);
			vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
		}
	}

	bool VulkanRaytracingPipeline::Build(RaytracingPipelineCreateInfo* info)
	{
		if(!BuildEX(info)) { return false; }


		{
			// Setup identity transform matrix
			VkTransformMatrixKHR transformMatrix = {
				1.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f
			};

			m_TransformBuffer.CreateBuffer(&transformMatrix, sizeof(VkTransformMatrixKHR),
				VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR);
		}

		// Dummy Scene 
		RaytracingPipelineSceneInfo sceneCI{};
		{
			glm::mat4 model;
			Utils::ComposeTransform(glm::vec3(0), glm::vec3(0), glm::vec3(1), model);
			sceneCI.Transforms = { model, };

			auto [cube, view] = MeshPool::GetCube();
			sceneCI.Scene.push_back({ cube, 1});

			CreateScene(&sceneCI);
		}
		
		// Descriptors
		{
			// Base
			VulkanPipeline::BuildDescriptors(m_Shader, info->NumDescriptorSets, m_Descriptors, m_DescriptorPool);

			// RT only
			for (auto& descriptor : m_Descriptors)
				descriptor.GenACStructureDescriptors(m_Shader, &m_TopLevelAS);
		}

		// Pipeline
		{
			VulkanShader* vkShader = m_Shader->Cast<VulkanShader>();
			VulkanDevice& device = VulkanContext::GetDevice();

			std::vector<VkDescriptorSetLayout> setLayouts;
			for (auto& descriptor : m_Descriptors)
			{
				setLayouts.push_back(descriptor.GetLayout());
			}

			VkPipelineLayoutCreateInfo pipelineLayoutCI = {};
			{
				pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
				pipelineLayoutCI.pNext = nullptr;
				pipelineLayoutCI.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
				pipelineLayoutCI.pSetLayouts = setLayouts.data();
				pipelineLayoutCI.pushConstantRangeCount = static_cast<uint32_t>(vkShader->m_VkPushConstantRanges.size());
				pipelineLayoutCI.pPushConstantRanges = vkShader->m_VkPushConstantRanges.data();

				VK_CHECK_RESULT(vkCreatePipelineLayout(device.GetLogicalDevice(), &pipelineLayoutCI, nullptr, &m_PipelineLayout));
			}

			/*
	           Create the ray tracing pipeline
            */

			VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCI{};
			rayTracingPipelineCI.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
			rayTracingPipelineCI.stageCount = static_cast<uint32_t>(vkShader->m_VkPipelineShaderStages.size());
			rayTracingPipelineCI.pStages = vkShader->m_VkPipelineShaderStages.data();
			rayTracingPipelineCI.groupCount = static_cast<uint32_t>(vkShader->m_ShaderGroupsRT.size());
			rayTracingPipelineCI.pGroups = vkShader->m_ShaderGroupsRT.data();
			rayTracingPipelineCI.maxPipelineRayRecursionDepth = 1;
			rayTracingPipelineCI.layout = m_PipelineLayout;

			VK_CHECK_RESULT(device.vkCreateRayTracingPipelinesKHR(device.GetLogicalDevice(), VK_NULL_HANDLE,
				VK_NULL_HANDLE, 1, &rayTracingPipelineCI, nullptr, &m_Pipeline));

			// SBT
			vkShader->CreateShaderBindingTable(m_Pipeline);
		}

		return true;
	}

	void VulkanRaytracingPipeline::CreateScene(RaytracingPipelineSceneInfo* info)
	{
		m_BottomLevelAS.clear();
		m_InstanceBuffer.Destroy();

		// Buttom Level
		for (auto& [topNode, instances] : info->Scene)
		{
			auto root = std::make_shared<BLAS>();
			root->IntanceCount = instances;

			for (auto& mesh : topNode->GetScene())
			{
				auto vb = mesh->GetVertexBuffer()->Cast<VulkanVertexBuffer>();
				auto ib = mesh->GetIndexBuffer()->Cast<VulkanIndexBuffer>();

				ObjDesc objDesc{};
				objDesc.materialUUID = 0;
				objDesc.vertexAddress = VulkanUtils::GetBufferDeviceAddress(vb->GetBuffer());
				objDesc.indexAddress = VulkanUtils::GetBufferDeviceAddress(ib->GetBuffer());;

				auto ac = std::make_shared<VulkanACStructure>();
				ac->BuildAsBottomLevel(m_Info.VertexInput.Stride, &m_TransformBuffer, mesh);

				m_ObjDescriptions.emplace_back(std::move(objDesc));
				root->Nodes.push_back(ac);
			}

			m_BottomLevelAS[topNode] = root;
		}

		// Top Level
		bool updateDescriptors = false;
		m_TopLevelAS.BuildAsTopLevel(&m_InstanceBuffer, info, m_BottomLevelAS, updateDescriptors);

		for (auto& descriptor : m_Descriptors)
		{
			descriptor.UpdateBuffer(666, sizeof(VulkanRaytracingPipeline::ObjDesc) * m_ObjDescriptions.size(), m_ObjDescriptions.data());
			m_ObjDescriptions.clear();
		}

		// Update Descriptors if needed (runtime)
		if (updateDescriptors)
		{
			for (auto& descriptor : m_Descriptors)
				descriptor.UpdateVkAccelerationStructure(m_Shader->GetACBindingPoint(), &m_TopLevelAS);
		}
	}

	void VulkanRaytracingPipeline::UpdateScene(RaytracingPipelineSceneInfo* info)
	{

	}

	void VulkanRaytracingPipeline::SubmitPushConstant(ShaderType stage, size_t size, const void* data)
	{
		vkCmdPushConstants(m_CommandBuffer, m_PipelineLayout, VulkanShader::GetVkShaderStage(stage), 0, static_cast<uint32_t>(size), data);
	}

	bool VulkanRaytracingPipeline::UpdateBuffer(uint32_t binding, size_t size, const void* data, uint32_t offset)
	{
		return m_Descriptors[m_DescriptorIndex].UpdateBuffer(binding, size, data, offset);
	}

	bool VulkanRaytracingPipeline::UpdateTextures(const std::vector<Ref<Texture>>& textures, uint32_t bindingPoint, TextureFlags usage)
	{
		return m_Descriptors[m_DescriptorIndex].UpdateTextures(textures, bindingPoint, usage);
	}

	bool VulkanRaytracingPipeline::UpdateTexture(const Ref<Texture>& texture, uint32_t bindingPoint, TextureFlags usage)
	{
		return m_Descriptors[m_DescriptorIndex].UpdateTexture(texture, bindingPoint, usage);
	}

	bool VulkanRaytracingPipeline::UpdateTexture(const Ref<Framebuffer>& fb, uint32_t bindingPoint, uint32_t attachmentIndex)
	{
		const auto& descriptor = fb->Cast<VulkanFramebuffer>()->GetAttachment(attachmentIndex)->ImageInfo;
		return m_Descriptors[m_DescriptorIndex].UpdateVkDescriptor(bindingPoint, descriptor);
	}

	bool VulkanRaytracingPipeline::UpdateTexture(const Ref<Framebuffer>& fb, uint32_t bindingPoint, const std::string& attachmentName)
	{
		const auto& descriptor = fb->Cast<VulkanFramebuffer>()->GetAttachment(attachmentName)->ImageInfo;
		return m_Descriptors[m_DescriptorIndex].UpdateVkDescriptor(bindingPoint, descriptor);
	}

	bool VulkanRaytracingPipeline::UpdateVkDescriptor(uint32_t bindingPoint, const void* descriptorPtr)
	{
		return m_Descriptors[m_DescriptorIndex].UpdateVkDescriptor(bindingPoint, *(VkDescriptorImageInfo*)descriptorPtr);
	}

	VkPipelineLayout VulkanRaytracingPipeline::GetVkPipelineLayout() const
	{
		return m_PipelineLayout;
	}

	VkPipeline VulkanRaytracingPipeline::GetVkPipeline() const
	{
		return m_Pipeline;
	}
}

#endif