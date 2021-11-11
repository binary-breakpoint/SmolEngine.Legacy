#pragma once
#include "Graphics/Primitives/GraphicsPipeline.h"

#include <string>

namespace SmolEngine
{
	struct RendererStorageBase;

	struct MaterialCreateInfo
	{
		std::string                Name = "";
		GraphicsPipelineCreateInfo PipelineCreateInfo{};
	};

	class Material
	{
	public:
		Material() = default;
		virtual ~Material() = default;

		void                      SetCommandBuffer(void* cmd);
		void*                     GetCommandBuffer();
		void                      DrawMeshIndexed(Ref<Mesh>& mesh, uint32_t instances = 1);
		void                      DrawMesh(Ref<Mesh>& mesh, uint32_t instances = 1);
		void                      SubmitPushConstant(ShaderType stage, size_t size, const void* data);
		bool                      UpdateBuffer(uint32_t binding, size_t size, const void* data, uint32_t offset = 0);
		bool                      UpdateTextures(const std::vector<Ref<Texture>>& textures, uint32_t binding);
		bool                      UpdateTexture(const Ref<Texture>& texture, uint32_t binding);
		Ref<GraphicsPipeline>     GetPipeline() const;
		uint32_t                  GetID() const;
		const std::string&        GetName() const;
		const MaterialCreateInfo& GetInfo() const;

	private:
		bool                      BuildEX(MaterialCreateInfo* ci, bool is2D);

	private:
		uint32_t                  m_ID = 0;
		MaterialCreateInfo        m_Info{};
		Ref<GraphicsPipeline>     m_Pipeline = nullptr;

		friend class Material2D;
		friend class Material3D;
	};
}