#pragma once
#ifndef OPENGL_IMPL

#include "Backends/Vulkan/VulkanDescriptor.h"
#include "Backends/Vulkan/VulkanCommandBuffer.h"
#include "Primitives/ComputePipeline.h"

namespace SmolEngine
{
	class VulkanComputePipeline: public ComputePipeline
	{
	public:
		bool                           Build(ComputePipelineCreateInfo* info) override;
		bool                           Reload() override;
		void                           BeginCompute(void* cmdStorage = nullptr) override;
		void                           EndCompute() override;
		void                           Execute(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) override;
		void                           Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ, uint32_t descriptorIndex = 0, void* descriptorSet = nullptr) override;
		void                           SubmitPushConstant(size_t size, const void* data) override;
		VulkanDescriptor&              GeDescriptor(uint32_t index = 0);

	private:
		VkDevice                       m_Device = nullptr;
		VkPipeline                     m_Pipeline = nullptr;
		VkPipelineCache                m_PipelineCache = nullptr;
		VkDescriptorPool               m_DescriptorPool = nullptr;
		VkPipelineLayout               m_PipelineLayout = nullptr;
		CommandBufferStorage           m_CmdStorage{};
		std::vector<VulkanDescriptor>  m_Descriptors;
	};
}

#endif