#pragma once
#ifndef OPENGL_IMPL

#include "Backends/Vulkan/VulkanDescriptor.h"

namespace SmolEngine
{
	struct ComputePipelineCreateInfo;
	class  VulkanShader;

	class VulkanComputePipeline
	{
	public:
		void                                BeginCompute(CommandBufferStorage* cmdStorage = nullptr);
		void                                BeginCompute(VkCommandBuffer cmdBuffer);
		void                                EndCompute();
		void                                Execute(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
		void                                Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ, uint32_t descriptorIndex = 0, VkDescriptorSet descriptorSet = nullptr);
		void                                SubmitPushConstant(size_t size, const void* data);
		VulkanDescriptor&                   GeDescriptor(uint32_t index = 0);

	private:
		bool                                Invalidate(ComputePipelineCreateInfo* pipelineSpec, VulkanShader* shader);

	private:
		VkDevice                            m_Device = nullptr;
		VkPipeline                          m_Pipeline = nullptr;
		VulkanShader*                       m_Shader = nullptr;
		VkPipelineCache                     m_PipelineCache = nullptr;
		VkDescriptorPool                    m_DescriptorPool = nullptr;
		VkPipelineLayout                    m_PipelineLayout = nullptr;
		ComputePipelineCreateInfo*          m_Spec = nullptr;
		CommandBufferStorage                m_CmdStorage{};
		std::vector<VulkanDescriptor>       m_Descriptors;

		friend class ComputePipeline;
	};
}

#endif