#include "stdafx.h"
#include "ECS/Components/Texture2DComponent.h"

#include "Pools/TexturePool.h"

namespace SmolEngine
{
	void Texture2DComponent::Load(const std::string& path)
	{
		Texture = TexturePool::ConstructFromPath(path);
		TexturePath = path;
	}

	const Ref<Texture>& Texture2DComponent::GetTexture() const
	{
		return Texture;
	}
}