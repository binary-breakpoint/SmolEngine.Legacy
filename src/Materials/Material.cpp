#include "stdafx.h"
#include "Materials/Material.h"
#include "Renderer/RendererDeferred.h"
#include "Renderer/Renderer2D.h"

namespace SmolEngine
{
	Ref<GraphicsPipeline> Material::GetPipeline() const
	{
		return m_Pipeline;
	}

	uint32_t Material::GetID() const
	{
		return m_ID;
	}

	bool Material::Build(MaterialCreateInfo* ci)
	{
		assert(!ci->Name.empty() || ci->pStorage != nullptr);

		if (ci->PipelineCreateInfo.ShaderCreateInfo.Stages.size() > 0)
		{
			Ref<Framebuffer> target = ci->bIs2D ? ci->pStorage->Cast<Renderer2DStorage>()->MainFB 
				: ci->pStorage->Cast<RendererStorage>()->f_GBuffer; // TODO: add main fb to base class

			ci->PipelineCreateInfo.TargetFramebuffers = { target };

			m_Pipeline = GraphicsPipeline::Create();
			m_Pipeline->Build(&ci->PipelineCreateInfo);
		}

		m_Info = *ci;
		return m_Pipeline != nullptr;
	}

	void Material::DrawMeshIndexed(Ref<Mesh>& mesh, uint32_t instances)
	{
		m_Pipeline->DrawMeshIndexed(mesh, instances);
	}

	void Material::DrawMesh(Ref<Mesh>& mesh, uint32_t instances)
	{
		m_Pipeline->DrawMesh(mesh, instances);
	}

	bool Material::SubmitBuffer(uint32_t binding, size_t size, const void* data, uint32_t offset)
	{
		return m_Pipeline->SubmitBuffer(binding, size, data, offset);
	}

	void Material::SubmitPushConstant(ShaderType stage, size_t size, const void* data)
	{
		m_Pipeline->SubmitPushConstant(stage, size, data);
	}

	bool Material::UpdateSamplers(const std::vector<Ref<Texture>>& textures, uint32_t binding)
	{
		return m_Pipeline->UpdateSamplers(textures, binding);
	}

	bool Material::UpdateSampler(Ref<Texture>& texture, uint32_t binding)
	{
		return m_Pipeline->UpdateSampler(texture, binding);
	}

	const std::string& Material::GetName() const
	{
		return m_Info.Name;
	}

	const MaterialCreateInfo& Material::GetInfo() const
	{
		return m_Info;
	}
}