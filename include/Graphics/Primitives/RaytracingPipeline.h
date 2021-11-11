#pragma once
#include "Graphics/Primitives/PrimitiveBase.h"
#include "Graphics/Primitives/PipelineBase.h"
#include "Graphics/Primitives/Shader.h"
#include "Graphics/Primitives/Texture.h"

#include "Graphics/Common/Vertex.h"

namespace SmolEngine
{
	struct RaytracingPipelineCreateInfo
	{
		int32_t NumDescriptorSets = 1;
		uint32_t VertexStride = 0;
		uint32_t MaxRayRecursionDepth = 1;
		std::string ShaderRayGenPath = "";
		std::string ShaderCloseHitPath = "";
		std::string ShaderMissPath = "";
		std::map<uint32_t, ShaderBufferInfo> Buffers;
	};

	struct RaytracingPipelineSceneInfo
	{
		std::vector<std::pair<Ref<Mesh>, std::vector<glm::mat4>>> Scene;
	};

	class RaytracingPipeline: public PrimitiveBase, public PipelineBase
	{
	public:
		RaytracingPipeline() = default;
		virtual ~RaytracingPipeline() = default;

		virtual void                   Free() = 0;
		virtual bool                   Build(RaytracingPipelineCreateInfo* info) = 0;
		virtual void                   CreateScene(RaytracingPipelineSceneInfo* info) = 0;
		virtual void                   UpdateScene(RaytracingPipelineSceneInfo* info) = 0;
		virtual void                   SetCommandBuffer(void* cmdStorage = nullptr) {};
		virtual void                   Dispatch(uint32_t width, uint32_t height) = 0;
		virtual void                   SubmitPushConstant(ShaderType stage, size_t size, const void* data) {};
		static Ref<RaytracingPipeline> Create();

	protected:
		bool                           BuildEX(RaytracingPipelineCreateInfo* info);

	protected:
		Ref<Shader>                  m_Shader = nullptr;
		RaytracingPipelineCreateInfo m_Info{};
	};
}