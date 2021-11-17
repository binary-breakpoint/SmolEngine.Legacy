#include "Asset/Asset.h"

namespace SmolEngine
{
	AssetType Asset::GetAssetType() const
	{
		return AssetType::None;
	}

	bool Asset::IsAssetValid() const
	{
		if ((m_Flags & AssetFlag::Missing) == AssetFlag::Missing)
			return false;

		if ((m_Flags & AssetFlag::Invalid) == AssetFlag::Invalid)
			return false;

		return true;
	}

	const size_t& Asset::GetUUID() const
	{
		return m_UUID;
	}

	bool Asset::operator==(const Asset& other) const
	{
		return m_UUID == other.m_UUID;
	}

	bool Asset::operator!=(const Asset& other) const
	{
		return !(*this == other);
	}
}