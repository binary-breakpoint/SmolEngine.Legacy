#include "stdafx.h"
#include "Common/CubeTexture.h"

namespace Frostium
{
	Ref<CubeTexture> Frostium::CubeTexture::Create(const std::string& filePath, TextureFormat format)
	{
		Ref<CubeTexture> texture = std::make_shared<CubeTexture>();
#ifdef  FROSTIUM_OPENGL_IMPL
#else
		texture->m_VulkanTetxure.LoadCubeMap(filePath, format);
#endif
		return texture;
	}
}
