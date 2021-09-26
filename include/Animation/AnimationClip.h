#pragma once

#include "Common/Common.h"
#include "Tools/GLM.h"

#include <string>

namespace cereal
{
	class access;
}

namespace SmolEngine
{
	struct AnimationClipInfo
	{
		float                     Speed = 1.0f;
		bool                      bLoop = true;
		bool                      bPlay = false;
	};

	struct AnimationClipCreateInfo
	{
		AnimationClipInfo         ClipInfo;
		std::string               SkeletonPath = "";
		std::string               AnimationPath = "";
		std::string               ModelPath = "";
		std::string               Name = "";

		bool Load(const std::string& filePath);
		bool Save(const std::string& filePath);

	private:
		friend class cereal::access;

		template<typename Archive>
		void serialize(Archive& archive)
		{
			archive(ClipInfo.bLoop, ClipInfo.bPlay, ClipInfo.Speed, SkeletonPath, AnimationPath, ModelPath, Name);
		}
	};

	struct AnimationClipStorage;

	class AnimationClip
	{
	public:
		bool                        IsGood() const;
		AnimationClipInfo&          GetProperties();
		float                       GetDuration() const;
		float                       GetTimeRatio() const;
		void                        SetTimeRatio(float ratio);
		void                        Reset();

	private:
		bool                        Update();
		bool                        Create(const AnimationClipCreateInfo& createInfo);
		void                        CopyJoints(std::vector<glm::mat4>& dist, uint32_t& out_index);

	private:
		float                       m_TimeRatio = 0.0f;
		float                       m_PreviousTimeRatio = 0.0f;
		float                       m_Duration = 0.0f;
		AnimationClipInfo           m_Info{};
		Ref<AnimationClipStorage>   m_Storage = nullptr;

		friend class AnimationController;
	};
}