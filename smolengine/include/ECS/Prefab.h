#pragma once

#include "Core/Core.h"

#include <string>
#include <vector>
#include <entt/entity/registry.hpp>
#include <glm/glm.hpp>

namespace SmolEngine
{
	class Actor;

	class Prefab
	{
	public:
		Ref<Actor> Instantiate(Scene* scene, const glm::vec3& pos, const glm::vec3& rot, const glm::vec3& scale);
		bool CreateFromActor(Ref<Actor>& actor, Scene* scene, const std::string& savePath);
		bool LoadFromFile(const std::string& filePath);
		bool IsGood() const;
		void Free();

	private:
		void FindEntityID(Ref<Actor>& actor, Scene* scene, std::vector<uint32_t>& out_ids);

	private:
		entt::registry m_Data{};
	};
}