#include "stdafx.h"
#include "Primitives/Texture.h"
#include "Primitives/Shader.h"

#include "Common/Core.h"
#include "Common/SLog.h"

#include "Utils/Utils.h"

#include <stb_image.h>
#include <memory>
#include <cereal/archives/json.hpp>

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	void Texture::Bind(uint32_t slot) const
	{
#ifdef  FROSTIUM_OPENGL_IMPL
		Bind(slot);
#endif
	}

	void Texture::UnBind() const
	{
#ifdef  FROSTIUM_OPENGL_IMPL
		UnBind();
#endif
	}

	bool Texture::IsReady() const
	{
		return m_Width > 0;
	}

	uint32_t Texture::GetHeight() const
	{
		return m_Height;
	}

	uint32_t Texture::GetWidth() const
	{
		return m_Width;
	}

	uint32_t Texture::GetID() const
	{
		return m_ID;
	}

	void* Texture::GetImGuiTexture() const
	{
#ifdef  FROSTIUM_OPENGL_IMPL
		return reinterpret_cast<void*>(GetID());
#else
		return GetImGuiTextureID();
#endif
	}

#ifndef FROSTIUM_OPENGL_IMPL
	VulkanTexture* Texture::GetVulkanTexture()
	{
		return dynamic_cast<VulkanTexture*>(this);
    }
#endif

	void Texture::Create(const TextureCreateInfo* info, Texture* out_texture)
	{
		std::hash<std::string_view> hasher{};
		out_texture->m_ID = static_cast<uint32_t>(hasher(info->FilePath));
#ifdef  FROSTIUM_OPENGL_IMPL
		out_texture->Init(filePath);
#else
		out_texture->LoadTexture(info);
#endif
	}

	void Texture::Create(const void* data, uint32_t size, const uint32_t width, const uint32_t height, Texture* out_texture, TextureFormat format)
	{
		if (out_texture)
		{
			std::hash<const void*> hasher{};
			out_texture->m_ID = static_cast<uint32_t>(hasher(data));
#ifdef  FROSTIUM_OPENGL_IMPL
#else
			out_texture->GenTexture(data, size, width, height, format);
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
			out_texture->Init(4, 4);
			out_texture->SetData(&whiteTextureData, sizeof(uint32_t));
#else
			out_texture->GenWhiteTetxure(4, 4);
#endif
		}
	}

	bool TextureCreateInfo::Save(const std::string& filePath)
	{
		std::stringstream storage;
		{
			cereal::JSONOutputArchive output{ storage };
			serialize(output);
		}

		std::ofstream myfile(filePath);
		if (myfile.is_open())
		{
			myfile << storage.str();
			myfile.close();
			return true;
		}

		return false;
	}

	bool TextureCreateInfo::Load(const std::string& filePath)
	{
		std::stringstream storage;
		std::ifstream file(filePath);
		if (!file)
		{
			NATIVE_ERROR("Could not open the file: {}", filePath);
			return false;
		}

		storage << file.rdbuf();
		{
			cereal::JSONInputArchive input{ storage };
			input(bVerticalFlip, bAnisotropyEnable, bImGUIHandle, eFormat, eAddressMode, eFilter, eBorderColor, FilePath);
		}

		return true;
	}
}