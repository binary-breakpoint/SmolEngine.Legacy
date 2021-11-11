#pragma once
#include "Core/Memory.h"

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

		virtual bool  UpdateBuffer(uint32_t binding, size_t size, const void* data, uint32_t offset = 0) = 0;
		virtual bool  UpdateTextures(const std::vector<Ref<Texture>>& textures, uint32_t bindingPoint, TextureFlags usage = TextureFlags::MAX_ENUM) = 0;
		virtual bool  UpdateTexture(const Ref<Texture>& texture, uint32_t bindingPoint, TextureFlags usage = TextureFlags::MAX_ENUM) = 0;
		virtual bool  UpdateTexture(const Ref<Framebuffer>& fb, uint32_t bindingPoint, uint32_t attachmentIndex = 0) = 0;
		virtual bool  UpdateTexture(const Ref<Framebuffer>& fb, uint32_t bindingPoint, const std::string& attachmentName) = 0;
		virtual bool  UpdateVkDescriptor(uint32_t bindingPoint, const void* descriptorPtr) { return false; }

	protected:
		uint32_t m_DescriptorIndex = 0;
	};
}