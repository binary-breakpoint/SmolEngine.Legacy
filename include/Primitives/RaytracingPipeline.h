#pragma once
#include "Primitives/PrimitiveBase.h"
#include "Primitives/PipelineBase.h"
#include "Primitives/Shader.h"
#include "Primitives/Texture.h"

#include "Common/Vertex.h"

namespace SmolEngine
{
	struct RaytracingPipelineCreateInfo
	{
		ShaderCreateInfo ShaderCI{};
		VertexInputInfo  VertexInput{};
		int32_t          NumDescriptorSets = 1;
	};

	class RaytracingPipeline: public PrimitiveBase, public PipelineBase
	{
	public:
		RaytracingPipeline() = default;
		virtual ~RaytracingPipeline() = default;

		virtual void                   Dispatch(uint32_t width, uint32_t height, void* cmdStorage = nullptr) = 0;
		virtual bool                   Build(RaytracingPipelineCreateInfo* info) = 0;
		virtual void                   SubmitPushConstant(ShaderType stage, size_t size, const void* data) {};
		static Ref<RaytracingPipeline> Create();

	protected:
		bool                           BuildEX(RaytracingPipelineCreateInfo* info);

	protected:
		Ref<Shader>  m_Shader = nullptr;
	};
}