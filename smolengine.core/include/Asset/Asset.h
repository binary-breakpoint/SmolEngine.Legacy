#pragma once
#include "Asset/AssetEnums.h"

namespace SmolEngine
{
	class Asset
	{
	public:
		bool                    IsAssetValid() const;
		const size_t&           GetUUID() const;
		virtual AssetType       GetAssetType() const;

		virtual bool operator==(const Asset& other) const;
		virtual bool operator!=(const Asset& other) const;

	protected:
		AssetType m_Type = AssetType::None;
		AssetFlag m_Flags = AssetFlag::None;
		size_t    m_UUID = 0;

		friend class AssetManager;
	};
}