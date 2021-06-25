#include "stdafx.h"
#include "Common/CubeMap.h"

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	void CubeMap::Create(CubeMap* out_tex, const std::string& filePath, TextureFormat format)
	{
		if (out_tex)
		{
			out_tex->m_Texture = std::make_shared<Texture>();
#ifdef  FROSTIUM_OPENGL_IMPL
#else
			out_tex->m_Texture->GetVulkanTexture()->LoadCubeMap(filePath, format);
#endif
		}
	}

	void CubeMap::CreateEmpty(CubeMap* out_tex, uint32_t width, uint32_t height, TextureFormat format)
	{
		if (out_tex)
		{
			out_tex->m_Texture = std::make_shared<Texture>();
#ifdef  FROSTIUM_OPENGL_IMPL
#else
			out_tex->m_Texture->GetVulkanTexture()->GenCubeMap(width, height, format);
#endif
		}
	}

	Texture* CubeMap::GetTexture() const
	{
		return m_Texture.get();
	}
}
