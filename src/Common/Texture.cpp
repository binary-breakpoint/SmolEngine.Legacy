#include "stdafx.h"
#include "Common/Texture.h"
#include "Common/Core.h"
#include "Common/SLog.h"
#include "Common/Shader.h"

#include "Utils/Utils.h"

#include <stb_image.h>
#include <memory>

namespace Frostium
{
	void Texture::Bind(uint32_t slot) const
	{
#ifdef  FROSTIUM_OPENGL_IMPL
		m_OpenglTexture2D.Bind(slot);
#endif
	}

	void Texture::UnBind() const
	{
#ifdef  FROSTIUM_OPENGL_IMPL
		m_OpenglTexture2D.UnBind();
#endif
	}

	bool Texture::IsInitialized() const
	{
#ifdef  FROSTIUM_OPENGL_IMPL
		return false;
#else
		return m_VulkanTexture.GetHeight() > 0;
#endif
	}

	uint32_t Texture::GetHeight() const
	{
#ifdef  FROSTIUM_OPENGL_IMPL
		return m_OpenglTexture2D.GetHeight();
#else
		return m_VulkanTexture.GetHeight();
#endif
	}

	uint32_t Texture::GetWidth() const
	{
#ifdef  FROSTIUM_OPENGL_IMPL
		return m_OpenglTexture2D.GetWidth();
#else
		return m_VulkanTexture.GetWidth();
#endif
	}

	const uint32_t Texture::GetID() const
	{
#ifdef  FROSTIUM_OPENGL_IMPL
		return m_OpenglTexture2D.GetID();
#else
		return static_cast<uint32_t>(m_VulkanTexture.GetID());
#endif
	}

	void* Texture::GetImGuiTexture() const
	{
#ifdef  FROSTIUM_OPENGL_IMPL
		return reinterpret_cast<void*>(m_OpenglTexture2D.GetID());
#else
		return m_VulkanTexture.GetImGuiTextureID();
#endif
	}

	void Texture::SetData(void* data, uint32_t size)
	{
#ifdef  FROSTIUM_OPENGL_IMPL
		m_OpenglTexture2D.SetData(data, size);
#else
#endif
	}

	void Texture::LoadTexture(const std::string& path, TextureLoadedData* outData)
	{
		outData->Data = nullptr;
		if (!Utils::IsPathValid(path))
			return;

		outData->FilePath = path;
		int height, width, channels;
		stbi_set_flip_vertically_on_load(1);
		stbi_uc* data = nullptr;
		{
			data = stbi_load(path.c_str(), &width, &height, &channels, 4);
			if (!data)
				NATIVE_ERROR("Texture not found! file: {}, line: {}", __FILE__, __LINE__);
		}

		outData->Data = data;
		outData->Height = height;
		outData->Width = width;
		outData->Channels = channels;
	}

	bool Texture::operator==(const Texture& other) const
	{
		return GetID() == other.GetID();
	}

	void Texture::Create(const std::string& filePath, Texture* out_texture, TextureFormat format, bool flip_vertically)
	{
		if (out_texture)
		{
			if (!Utils::IsPathValid(filePath))
				return;

#ifdef  FROSTIUM_OPENGL_IMPL
			out_texture->m_OpenglTexture2D.Init(filePath);
#else
			out_texture->m_VulkanTexture.LoadTexture(filePath, flip_vertically, format);
#endif
		}
	}

	void Texture::Create(const void* data, uint32_t size, const uint32_t width, const uint32_t height, Texture* out_texture, TextureFormat format)
	{
		if (out_texture)
		{
#ifdef  FROSTIUM_OPENGL_IMPL
#else
			out_texture->m_VulkanTexture.GenTexture(data, size, width, height, format);
#endif
		}
	}

	void Texture::CreateWhiteTexture(Texture* out_texture)
	{
		if (out_texture)
		{
#ifdef  FROSTIUM_OPENGL_IMPL
			uint32_t whiteTextureData = 0xffffffff;
			out_texture->m_OpenglTexture2D.Init(4, 4);
			out_texture->m_OpenglTexture2D.SetData(&whiteTextureData, sizeof(uint32_t));
#else
			out_texture->m_VulkanTexture.GenWhiteTetxure(4, 4);
#endif
		}
	}
}