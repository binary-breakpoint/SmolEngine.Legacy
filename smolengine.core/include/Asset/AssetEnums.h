#pragma once
#include "Memory.h"

namespace SmolEngine
{
	enum class AssetFlag : int
	{
		None    = 2,
		Missing = 2,
		Invalid = 4
	};

	inline AssetFlag operator~ (AssetFlag a) { return (AssetFlag)~(int)a; }
	inline AssetFlag operator| (AssetFlag a, AssetFlag b) { return (AssetFlag)((int)a | (int)b); }
	inline AssetFlag operator& (AssetFlag a, AssetFlag b) { return (AssetFlag)((int)a & (int)b); }
	inline AssetFlag operator^ (AssetFlag a, AssetFlag b) { return (AssetFlag)((int)a ^ (int)b); }
	inline AssetFlag& operator|= (AssetFlag& a, AssetFlag b) { return (AssetFlag&)((int&)a |= (int)b); }
	inline AssetFlag& operator&= (AssetFlag& a, AssetFlag b) { return (AssetFlag&)((int&)a &= (int)b); }
	inline AssetFlag& operator^= (AssetFlag& a, AssetFlag b) { return (AssetFlag&)((int&)a ^= (int)b); }

	enum class AssetType : uint16_t
	{
		None = 0,
		Prefab = 2,
		Mesh = 3,
		Material = 4,
		Texture = 5,
		Audio = 6,
		PhysicsMaterial = 7,
	};

	inline const char* AssetTypeToString(AssetType assetType)
	{
		switch (assetType)
		{
		case AssetType::Prefab:                return "Prefab";
		case AssetType::Mesh:                  return "Mesh";
		case AssetType::Material:              return "Material";
		case AssetType::Texture:               return "Texture";
		case AssetType::Audio:                 return "Audio";
		case AssetType::PhysicsMaterial:       return "Audio";
		}

		return "None";
	}

	inline const char* AssetFlagToString(AssetFlag flag)
	{
		switch (flag)
		{
		case AssetFlag::Missing:  return "Missing";
		case AssetFlag::Invalid:  return "Invalid";
		}

		return "None";
	}
}