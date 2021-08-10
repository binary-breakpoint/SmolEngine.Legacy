#include "stdafx.h"
#include "Environment/CubeMap.h"

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	void CubeMap::Create(const TextureCreateInfo* info, CubeMap* out_tex)
	{
#ifdef  FROSTIUM_OPENGL_IMPL
#else
		out_tex->GetVulkanTexture()->LoadCubeMap(info);
#endif
	}

	void CubeMap::CreateEmpty(CubeMap* out_tex, uint32_t width, uint32_t height, TextureFormat format)
	{
		if (out_tex)
		{
#ifdef  FROSTIUM_OPENGL_IMPL
#else
			out_tex->GetVulkanTexture()->GenCubeMap(width, height, format);
#endif
		}
	}

	Texture* CubeMap::GetTexture()
	{
		return dynamic_cast<Texture*>(this);
	}
}
