#pragma once

#include "Core/Core.h"

#include <string>
#include <unordered_map>
#include <mutex>

namespace SmolEngine
{
	class Mesh;
	class Texture;
	class Prefab;

	struct AudioClip;
	struct AudioClipCreateInfo;
	struct TextureCreateInfo;

	class AssetManager
	{
	public:
		size_t          AddMesh(const std::string& filePath, Ref<Mesh>& out_object);
		size_t          AddTexture(const std::string& filePath, Ref<Texture>& out_object);
		size_t          AddPrefab(const std::string& filePath, Ref<Prefab>& out_object);
		size_t          AddAudioClip(AudioClipCreateInfo* create_info, Ref<AudioClip>& out_object);

		Ref<Mesh>       GetMesh(const size_t& id);
		Ref<Texture>    GetTexture(const size_t& id);
		Ref<Prefab>     GetPrefab(const size_t& id);
		Ref<AudioClip>  GetAudioClip(const size_t& id);
		bool            GetPath(const size_t& id, std::string& out_path);
		void            OnReset();

	private:
		void AddPath(const std::string& path, const size_t& id);

	private:

		std::mutex                                 m_Mutex{};
		std::hash<std::string>                     m_Hasher{};
		std::unordered_map<size_t, std::string>    m_HashMap;
		std::unordered_map<size_t, Ref<Mesh>>      m_MeshMap;
		std::unordered_map<size_t, Ref<Texture>>   m_TextureMap;
		std::unordered_map<size_t, Ref<Prefab>>    m_PrefabMap;
		std::unordered_map<size_t, Ref<AudioClip>> m_AudioClipMap;
	};
}