#include "stdafx.h"
#include "Materials/MaterialPBR.h"
#include "Materials/PBRFactory.h"
#include "Renderer/RendererDeferred.h"

namespace SmolEngine
{
	bool MaterialPBR::Initialize()
	{
		GraphicsPipelineCreateInfo pipelineCI = {};
		MaterialCreateInfo materialCI{};
		ShaderCreateInfo shaderCI = {};

		{
			const auto& path = GraphicsContext::GetSingleton()->GetResourcesPath();

			shaderCI.Stages[ShaderType::Vertex] = path + "Shaders/Gbuffer.vert";
			shaderCI.Stages[ShaderType::Fragment] = path + "Shaders/Gbuffer.frag";

			// SSBO's
			ShaderBufferInfo bufferInfo = {};
			RendererStorage* storage = RendererStorage::GetSingleton();

			// Vertex
			bufferInfo.Size = sizeof(InstanceData) * max_objects;
			shaderCI.Buffers[storage->m_ShaderDataBinding] = bufferInfo;

			bufferInfo.Size = sizeof(PBRUniform) * max_materials;
			shaderCI.Buffers[storage->m_MaterialsBinding] = bufferInfo;

			bufferInfo.Size = sizeof(glm::mat4) * max_anim_joints;
			shaderCI.Buffers[storage->m_AnimBinding] = bufferInfo;
		}

		{
			pipelineCI.VertexInputInfos = { GetVertexInputInfo() };
			pipelineCI.PipelineName = "PBR";
			pipelineCI.ShaderCreateInfo = shaderCI;
		}

		materialCI.Name = pipelineCI.PipelineName;
		materialCI.PipelineCreateInfo = pipelineCI;

		return Build(&materialCI);
	}

	Ref<MaterialPBR> MaterialPBR::Create()
	{
		Ref<MaterialPBR> material = std::make_shared<MaterialPBR>();
		material->Initialize();
		return material;
	}
}