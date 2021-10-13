#pragma once
#include "Primitives/GraphicsPipeline.h"

#include <string>

namespace SmolEngine
{
	struct RendererStorageBase;

	struct MaterialCreateInfo
	{
		std::string                Name = "";
		RendererStorageBase*       pStorage = nullptr;
		GraphicsPipelineCreateInfo PipelineCreateInfo{};

	private:
		bool                       bIs2D = false;

		friend class Material;
		friend class Material2D;
	};

	class Material
	{
	public:
		Material() = default;
		virtual ~Material() = default;

		bool                      Build(MaterialCreateInfo* ci);
		bool                      SubmitBuffer(uint32_t binding, size_t size, const void* data, uint32_t offset = 0);
		void                      SubmitPushConstant(ShaderType stage, size_t size, const void* data);
		bool                      UpdateSamplers(const std::vector<Ref<Texture>>& textures, uint32_t binding);
		bool                      UpdateSampler(Ref<Texture>& texture, uint32_t binding);
		Ref<GraphicsPipeline>     GetPipeline() const;
		const std::string&        GetName() const;
		const MaterialCreateInfo& GetInfo() const;

	private:
		MaterialCreateInfo        m_Info{};
		Ref<GraphicsPipeline>     m_Pipeline = nullptr;
	};
}