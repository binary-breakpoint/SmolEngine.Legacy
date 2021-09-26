#include "stdafx.h"
#include "Primitives/Texture.h"
#include "Primitives/Shader.h"

#include "Common/Common.h"
#include "Common/DebugLog.h"

#include "Tools/Utils.h"

#include <memory>
#include <cereal/cereal.hpp>
#include <cereal/archives/json.hpp>

#include <stb_image/stb_image.h>

namespace SmolEngine
{
	Texture::Texture()
#ifdef  FROSTIUM_OPENGL_IMPL
#else
		:VulkanTexture(&m_Info)
#endif
	{

	}

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
		return m_Info.Height > 0;
	}

	const TextureInfo& Texture::GetInfo() const
	{
		return m_Info;
	}

	void* Texture::GetImGuiTexture() const
	{
		return m_Info.ImHandle;
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
		out_texture->m_Info.ID = static_cast<uint32_t>(hasher(info->FilePath));

#ifdef  FROSTIUM_OPENGL_IMPL
		out_texture->Init(filePath);
#else
		out_texture->LoadTexture(info);
#endif
	}

	void Texture::Create(const void* data, uint32_t size, uint32_t width, uint32_t height, Texture* out_texture, TextureFormat format)
	{
		std::hash<const void*> hasher{};
		out_texture->m_Info.ID = static_cast<uint32_t>(hasher(data));

#ifdef  FROSTIUM_OPENGL_IMPL
#else
		out_texture->GenTexture(data, size, width, height, format);
#endif
	}

	void Texture::CreateWhiteTexture(Texture* out_texture)
	{
		std::hash<std::string_view> hasher{};
		out_texture->m_Info.ID = static_cast<uint32_t>(hasher("WhiteTexture"));

#ifdef  FROSTIUM_OPENGL_IMPL
		uint32_t whiteTextureData = 0xffffffff;
		out_texture->Init(4, 4);
		out_texture->SetData(&whiteTextureData, sizeof(uint32_t));
#else
		out_texture->GenWhiteTetxure(4, 4);
#endif
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
			DebugLog::LogError("Could not open the file: {}", filePath);
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