#pragma once

#include "Common/Common.h"
#include "Utils/GLM.h"

#include <string>

namespace cereal
{
	class access;
}

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	struct AnimationClipCreateInfo
	{
		float        Speed = 0.0f;
		std::string  SkeletonPath = "";
		std::string  AnimationPath = "";
		std::string  ModelPath = "";
		bool         bLoop = true;

		void Load();
		void Save();

	private:
		friend class cereal::access;

		template<typename Archive>
		void serialize(Archive& archive)
		{
			archive(bLoop, Speed, SkeletonPath, AnimationPath);
		}
	};

	struct AnimationClipStorage;

	class AnimationClip
	{
	public:
		bool                    IsGood() const;
		// Setters
		void                    SetLoop(bool value);
		void                    SetSpeed(float value);
		// Getters
		bool                    IsLooping() const;
		float                   GetSpeed() const;
		std::vector<glm::mat4>& GetJoints() const;

	private:
		bool   Update();
		bool   Create(const AnimationClipCreateInfo& createInfo);

	private:
		Ref<AnimationClipStorage> m_Storage;

		friend class AnimationController;
	};
}