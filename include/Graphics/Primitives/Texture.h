#pragma once
#include "Core/Memory.h"
#include "Graphics/Primitives/PrimitiveBase.h"

#include <glm/glm.hpp>
#include <string>

namespace cereal
{
	class access;
}

namespace SmolEngine
{
	enum class ImageFilter : int
	{
		NEAREST,
		LINEAR,
	};

	enum class TextureFormat : int
	{
		R32_INT,
		R32_UINT,

		R8_UNORM,
		R8G8B8A8_UNORM,
		B8G8R8A8_UNORM,

		// HDR
		R16G16B16A16_SFLOAT,
		R32G32B32A32_SFLOAT,

		R32G32B32A32_UINT,
		R32G32B32A32_INT
	};

	enum class AddressMode: int
	{
		REPEAT,
		MIRRORED_REPEAT,
		CLAMP_TO_EDGE,
		CLAMP_TO_BORDER,
		MIRROR_CLAMP_TO_EDGE,
	};

	enum class BorderColor : int
	{
		FLOAT_TRANSPARENT_BLACK,
		INT_TRANSPARENT_BLACK,
		FLOAT_OPAQUE_BLACK,
		INT_OPAQUE_BLACK,
		FLOAT_OPAQUE_WHITE,
		INT_OPAQUE_WHITE
	};

	struct TextureInfo
	{
		uint32_t ID = 0;
		uint32_t Width = 0;
		uint32_t Height = 0;
		void*    ImHandle = nullptr; // imgui texture id
	};

	struct TextureCreateInfo
	{
		bool           bVerticalFlip = true;
		bool           bAnisotropyEnable = true;
		bool           bImGUIHandle = false;
		TextureFormat  eFormat = TextureFormat::R8G8B8A8_UNORM;
		AddressMode    eAddressMode = AddressMode::REPEAT;
		ImageFilter    eFilter = ImageFilter::LINEAR;
		BorderColor    eBorderColor = BorderColor::FLOAT_OPAQUE_WHITE;
		std::string    FilePath = "";
		uint32_t       Width = 0;
		uint32_t       Height = 0;
		uint32_t       Mips = 0;
		uint32_t       Depth = 1;

	public:
		bool           Save(const std::string& filePath);
		bool           Load(const std::string& filePath);

	private:
		friend class cereal::access;

		template<typename Archive>
		void serialize(Archive& archive)
		{
			archive(bVerticalFlip, bAnisotropyEnable, bImGUIHandle, eFormat, eAddressMode, eFilter, eBorderColor, Width, Height, Mips, Depth, FilePath);
		}
	};

	enum class TextureFlags
	{
		SAMPLER_2D = 1,
		SAMPLER_3D = 2,
		IMAGE_2D   = 4,
		IMAGE_3D   = 8,
		CUBEMAP    = 16,

		MAX_ENUM   = 32,
	};

	class Texture: public PrimitiveBase
	{
	public:
		virtual ~Texture() = default;

		virtual void                          LoadFromFile(TextureCreateInfo* info) = 0;
		virtual void                          LoadFromMemory(const void* data, uint32_t size, TextureCreateInfo* info) = 0;
		virtual void                          LoadAsCubeFromKtx(TextureCreateInfo* info) = 0;
		virtual void                          LoadAsWhiteCube(TextureCreateInfo* info) = 0;
		virtual void                          LoadAsStorage(TextureCreateInfo* info) = 0;
		virtual void                          LoadAs3D(TextureCreateInfo* info) = 0;
		virtual void                          LoadAsWhite() = 0;
		virtual void                          ClearImage(void* cmdBuffer) = 0;

		virtual uint32_t                      GetMips() const { return 0; };
		virtual std::pair<uint32_t, uint32_t> GetMipSize(uint32_t mip) const { return { 0, 0 }; };
		size_t                                GetID() const { return m_ID; }
		const TextureInfo&                    GetInfo() const { return m_Info; }
		void*                                 GetImGuiTexture() const { return m_Info.ImHandle; }
		bool                                  IsGood() const override { return m_Info.Width > 0; }
		TextureFlags                          GetFlags() const { return m_eFlags; }
		// Factory
		static Ref<Texture>                   Create();

	protected:
		TextureFlags m_eFlags = TextureFlags::SAMPLER_2D;
		TextureInfo  m_Info{};
		size_t       m_ID = 0;
	};
}
