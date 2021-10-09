#include "stdafx.h"
#include "Primitives/Texture.h"
#include "Primitives/Shader.h"
#include "Common/DebugLog.h"

#include "Tools/Utils.h"

#include <memory>
#include <cereal/cereal.hpp>
#include <cereal/archives/json.hpp>

#include <stb_image/stb_image.h>

#ifdef OPENGL_IMPL
#include "Backends/OpenGL/OpenglTexture.h"
#else
#include "Backends/Vulkan/VulkanTexture.h"
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
			input(bVerticalFlip, bAnisotropyEnable, bImGUIHandle, bIsCube, eFormat, eAddressMode, eFilter, eBorderColor, Width, Height, Mips, FilePath);
		}

		return true;
	}
}