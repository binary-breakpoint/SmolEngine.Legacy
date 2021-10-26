#pragma once
#include "Common/Memory.h"

#include <vector>
#include <string>

namespace SmolEngine
{
	class Texture;
	class Framebuffer;

	enum class ShaderType : int;

	class PipelineBase
	{
	public:
		virtual ~PipelineBase() = default;

		void          SetDescriptorIndex(uint32_t value) { m_DescriptorIndex = value; }

		virtual bool  SubmitBuffer(uint32_t binding, size_t size, const void* data, uint32_t offset = 0) = 0;
		virtual bool  UpdateSamplers(const std::vector<Ref<Texture>>& textures, uint32_t bindingPoint, bool storageImage = false) = 0;
		virtual bool  UpdateSampler(Ref<Texture>& tetxure, uint32_t bindingPoint, bool storageImage = false) = 0;
		virtual bool  UpdateSampler(Ref<Framebuffer>& framebuffer, uint32_t bindingPoint, uint32_t attachmentIndex = 0) = 0;
		virtual bool  UpdateSampler(Ref<Framebuffer>& framebuffer, uint32_t bindingPoint, const std::string& attachmentName) = 0;
		virtual bool  UpdateImageDescriptor(uint32_t bindingPoint, void* descriptor) = 0;
		virtual bool  UpdateCubeMap(Ref<Texture>& cubeMap, uint32_t bindingPoint) = 0;

	protected:
		uint32_t m_DescriptorIndex = 0;
	};
}