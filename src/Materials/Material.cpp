#include "stdafx.h"
#include "Materials/Material.h"
#include "Renderer/RendererDeferred.h"
#include "Renderer/Renderer2D.h"

#include "Backends/Vulkan/VulkanPipeline.h"

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

	bool Material::BuildEX(MaterialCreateInfo* ci, bool is2D)
	{
		assert(!ci->Name.empty());

		if (ci->PipelineCreateInfo.ShaderCreateInfo.Stages.size() > 0)
		{
			Ref<Framebuffer> target = is2D ? Renderer2DStorage::GetSingleton()->MainFB : RendererStorage::GetSingleton()->f_GBuffer;

			ci->PipelineCreateInfo.TargetFramebuffers = { target };
			ci->PipelineCreateInfo.PipelineName = ci->Name;
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

	void Material::SetCommandBuffer(void* cmd)
	{
		m_Pipeline->SetCommandBuffer(cmd);
	}

	void* Material::GetCommandBuffer()
	{
		return m_Pipeline->GetCommandBuffer();
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