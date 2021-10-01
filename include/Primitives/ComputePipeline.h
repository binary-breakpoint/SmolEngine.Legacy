#pragma once
#include "Primitives/Shader.h"

namespace SmolEngine
{
	struct ComputePipelineCreateInfo
	{
		uint32_t                                         DescriptorCount = 1;
		std::string                                      ShaderPath = "";
		std::unordered_map<uint32_t, ShaderBufferInfo>   ShaderBufferInfos;
	};

	class ComputePipeline
	{
	public:
		virtual bool                Build(ComputePipelineCreateInfo* info) = 0;
		virtual bool                Reload() = 0;
		virtual void                BeginCompute(void* cmdStorage = nullptr) = 0;
		virtual void                EndCompute() = 0;
		virtual void                Execute(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) = 0;
		virtual void                Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ, uint32_t descriptorIndex = 0, void* descriptorSet = nullptr) = 0;
		virtual void                SubmitPushConstant(size_t size, const void* data) {};
		static Ref<ComputePipeline> Create();

		template<typename T>
		T* Cast() { return dynamic_cast<T*>(this); }

	private:
		bool                        BuildBase(ComputePipelineCreateInfo* info);

	private:
		Ref<Shader>                m_Shader = nullptr;
		ComputePipelineCreateInfo  m_Info{};

		friend class VulkanComputePipeline;
	};
}