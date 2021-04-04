#pragma once

#ifdef FROSTIUM_OPENGL_IMPL
#include "OpenGL/OpenglTexture.h"
#else
#include "Vulkan/VulkanTexture.h"
#endif

#include "Common/Shared.h"

#include <glm/glm.hpp>
#include <string>

namespace Frostium
{
	struct TextureLoadedData;

	class Texture
	{
	public:

		Texture() = default;

		~Texture() = default;

		/// Binding

		void Bind(uint32_t slot = 0) const;

		void UnBind() const;

		/// Getters

		uint32_t GetHeight() const;

		uint32_t GetWidth() const;

		const uint32_t GetID() const;

		void* GetImGuiTexture() const;

#ifndef  FROSTIUM_OPENGL_IMPL

		VulkanTexture* GetVulkanTexture() { return &m_VulkanTexture; }
#endif
		/// Setters

		void SetData(void* data, uint32_t size);

		///

		static void LoadTexture(const std::string& path, TextureLoadedData* outData);

		/// Factory

		static Ref<Texture> Create(const std::string& filePath, TextureFormat format = TextureFormat::R8G8B8A8_UNORM);

		static Ref<Texture> Create(const TextureLoadedData* data, TextureFormat format = TextureFormat::R8G8B8A8_UNORM);

		static Ref<Texture> Create(const void* data, uint32_t size, const uint32_t width, const uint32_t height,
			TextureFormat format = TextureFormat::R8G8B8A8_UNORM);

		static Ref<Texture> CreateWhiteTexture();

		static void Create(const std::string& filePath, Texture* out_texture, TextureFormat format = TextureFormat::R8G8B8A8_UNORM);

		static void Create(const TextureLoadedData* data, Texture* out_texture, TextureFormat format = TextureFormat::R8G8B8A8_UNORM);

		static void Create(const void* data, uint32_t size, const uint32_t width, const uint32_t height, Texture* out_texture,
			TextureFormat format = TextureFormat::R8G8B8A8_UNORM);

		static void CreateWhiteTexture(Texture* out_texture);

		/// Operators

		bool operator==(const Texture& other) const;

	private:

#ifdef  FROSTIUM_OPENGL_IMPL
		OpenglTexture2D m_OpenglTexture2D = {};
#else
		VulkanTexture m_VulkanTexture = {};
#endif
		bool m_Initialized = false;
	};
}
