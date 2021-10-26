#pragma once
#include "Primitives/PrimitiveBase.h"
#include "Primitives/PipelineBase.h"
#include "Primitives/Shader.h"

namespace SmolEngine
{
	struct RaytracingPipelineCreateInfo
	{
		ShaderCreateInfo ShaderCI{};
	};

	class RaytracingPipeline: public PrimitiveBase, public PipelineBase
	{
	public:
		RaytracingPipeline() = default;
		virtual ~RaytracingPipeline() = default;

		virtual bool                   Build(RaytracingPipelineCreateInfo* info) = 0;
		virtual void                   SubmitPushConstant(ShaderType stage, size_t size, const void* data) {};

		static Ref<RaytracingPipeline> Create();

	protected:
		Ref<Shader> m_Shader = nullptr;
	};
}