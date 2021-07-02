#include "stdafx.h"
#include "Primitives/Texture.h"
#include "Primitives/Shader.h"

#include "Common/Core.h"
#include "Common/SLog.h"

#include "Utils/Utils.h"

#include <stb_image.h>
#include <memory>

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
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
		return false; // temp
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
		return m_ID;
	}

	void* Texture::GetImGuiTexture() const
	{
#ifdef  FROSTIUM_OPENGL_IMPL
		return reinterpret_cast<void*>(m_OpenglTexture2D.GetID());
#else
		return m_VulkanTexture.GetImGuiTextureID();
#endif
	}

	bool Texture::operator==(const Texture& other) const
	{
		return m_ID == other.m_ID;
	}

	void Texture::Create(const std::string& filePath, Texture* out_texture, TextureFormat format, bool flip,  bool imgui_handler)
	{
		if (out_texture && Utils::IsPathValid(filePath))
		{
			std::hash<std::string_view> hasher{};
			out_texture->m_ID = static_cast<uint32_t>(hasher(filePath));
#ifdef  FROSTIUM_OPENGL_IMPL
			out_texture->m_OpenglTexture2D.Init(filePath);
#else
			out_texture->m_VulkanTexture.LoadTexture(filePath, format, flip, imgui_handler);
#endif
		}
	}

	void Texture::Create(const void* data, uint32_t size, const uint32_t width, const uint32_t height, Texture* out_texture, TextureFormat format)
	{
		if (out_texture)
		{
			std::hash<const void*> hasher{};
			out_texture->m_ID = static_cast<uint32_t>(hasher(data));

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
			std::hash<std::string_view> hasher{};
			out_texture->m_ID = static_cast<uint32_t>(hasher("WhiteTexture"));
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