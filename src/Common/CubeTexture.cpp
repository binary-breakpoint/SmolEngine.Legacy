#include "stdafx.h"
#include "Common/CubeTexture.h"

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	Ref<CubeTexture> CubeTexture::Create(const std::string& filePath, TextureFormat format)
	{
		Ref<CubeTexture> texture = std::make_shared<CubeTexture>();
#ifdef  FROSTIUM_OPENGL_IMPL
#else
		texture->m_VulkanTetxure.LoadCubeMap(filePath, format);
#endif
		return texture;
	}
}
