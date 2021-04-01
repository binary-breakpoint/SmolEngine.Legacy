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

	Ref<Texture> Texture::CreateWhiteTexture()
	{
		Ref<Texture> texture = std::make_shared<Texture>();
#ifdef  FROSTIUM_OPENGL_IMPL
		uint32_t whiteTextureData = 0xffffffff;
		texture->m_OpenglTexture2D.Init(w, h);
		texture->m_OpenglTexture2D.SetData(&whiteTextureData, sizeof(uint32_t));
#else
		texture->m_VulkanTexture.GenWhiteTetxure(1, 1);
#endif
		texture->m_Initialized = true;

		return texture;
	}

	Ref<Texture> Texture::Create(const std::string& filePath, TextureFormat format, bool pooling)
	{
		if (!Utils::IsPathValid(filePath))
			return nullptr;

		Ref<Texture> texture = std::make_shared<Texture>();
#ifdef  FROSTIUM_OPENGL_IMPL
		texture->m_OpenglTexture2D.Init(filePath);
#else
		texture->m_VulkanTexture.LoadTexture(filePath, format);
#endif
		texture->m_Initialized = true;
		return texture;
	}

	Ref<Texture> Texture::Create(const TextureLoadedData* data, TextureFormat format, bool pooling)
	{
		if (!data)
			return nullptr;

		if (data->Data == nullptr)
			return nullptr;

		Ref<Texture> texture = std::make_shared<Texture>();
#ifdef  FROSTIUM_OPENGL_IMPL
		texture->m_OpenglTexture2D.Init(filePath);
#else
		texture->m_VulkanTexture.LoadTexture(data, format);
#endif
		stbi_image_free(data->Data);
		texture->m_Initialized = true;
		return texture;
	}

	Ref<Texture> Texture::Create(const void* data, uint32_t size, const uint32_t width,
		const uint32_t height, TextureFormat format)
	{
		Ref<Texture> texture = nullptr;
#ifdef  FROSTIUM_OPENGL_IMPL
#else
		texture = std::make_shared<Texture>();
		texture->m_VulkanTexture.GenTexture(data, size, width, height, format);

		texture->m_Initialized = true;
#endif
		return texture;
	}
}