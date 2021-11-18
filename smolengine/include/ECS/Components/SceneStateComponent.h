#pragma once

#include "ECS/Actor.h"

#include <string>
#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>

#include <unordered_map>
#include <vector>
#include <string>

namespace cereal
{
	class access;
}

namespace SmolEngine
{
	enum class SceneState
	{
		None =  0,
		Build = 1,
		Simulation = 2,
	};

	struct SceneStateComponent
	{
		SceneStateComponent() = default;

		SceneState                                     eState = SceneState::None;
		uint32_t                                       SceneID = 0;
		uint32_t                                       LastActorID = 1;
		std::string                                    FilePath = "";
		std::string                                    Name = "";
		std::unordered_map<std::string, Ref<Actor>>    ActorNameSet;
		std::unordered_map<uint32_t, Ref<Actor>>       ActorIDSet;
		std::vector<Ref<Actor>>                        Actors;
		std::vector<Ref<Actor>>                        ResetList;

	private:
		friend class cereal::access;
		template<typename Archive>
		void serialize(Archive& archive)
		{
			archive(SceneID, LastActorID, FilePath, Name, ActorNameSet, ActorIDSet, Actors);
		}
	};
}