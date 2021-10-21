#pragma once
#include "Primitives/Shader.h"
#include "Primitives/PrimitiveBase.h"
#include "Primitives/PipelineBase.h"

namespace SmolEngine
{
	struct ComputePipelineCreateInfo
	{
		uint32_t                     DescriptorCount = 1;
		std::string                  ShaderPath = "";
		std::unordered_map<uint32_t, 
			ShaderBufferInfo>        ShaderBufferInfos;
	};

	class ComputePipeline: public PrimitiveBase, public PipelineBase
	{
	public:
		virtual ~ComputePipeline() = default;

		virtual bool                Build(ComputePipelineCreateInfo* info) = 0;
		virtual bool                Reload() = 0;
		virtual void                BeginCompute(void* cmdStorage = nullptr) = 0;
		virtual void                EndCompute() = 0;
		virtual void                Execute(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) = 0;
		virtual void                Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ, void* descriptorSet = nullptr) = 0;
		virtual void                SubmitPushConstant(size_t size, const void* data) {};
		void                        SetDescriptorIndex(uint32_t index);
		static Ref<ComputePipeline> Create();

	private:
		bool                        BuildBase(ComputePipelineCreateInfo* info);

	private:
		Ref<Shader>                m_Shader = nullptr;
		uint32_t                   m_DescriptorIndex = 0;
		ComputePipelineCreateInfo  m_Info{};

		friend class VulkanComputePipeline;
	};
}