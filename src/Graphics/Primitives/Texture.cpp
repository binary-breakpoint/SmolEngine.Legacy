#include "stdafx.h"
#include "Graphics/Primitives/Texture.h"
#include "Graphics/Primitives/Shader.h"

#include "Graphics/Tools/Utils.h"

#include <memory>
#include <cereal/cereal.hpp>
#include <cereal/archives/json.hpp>

#include <stb_image/stb_image.h>

#ifdef OPENGL_IMPL
#include "Graphics/Backends/OpenGL/OpenglTexture.h"
#else
#include "Graphics/Backends/Vulkan/VulkanTexture.h"
#endif

namespace SmolEngine
{
	Ref<Texture> Texture::Create()
	{
		Ref<Texture> texture = nullptr;
#ifdef OPENGL_IMPL
#else
		texture = std::make_shared<VulkanTexture>();
#endif 
		return texture;
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
			input(bVerticalFlip, bAnisotropyEnable, bImGUIHandle, eFormat, eAddressMode, eFilter, eBorderColor, Width, Height, Mips, Depth, FilePath);
		}

		return true;
	}
}