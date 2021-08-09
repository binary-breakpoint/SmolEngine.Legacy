#include "stdafx.h"
#include "Animation/AnimationController.h"

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	bool AnimationController::AddClip(const AnimationClipCreateInfo& info, const std::string& name)
	{
		bool found = m_Clips.find(name) != m_Clips.end();
		if (!found)
		{
			Ref<AnimationClip> clip = std::make_shared<AnimationClip>();
			if (clip->Create(info))
			{
				m_Clips[name] = clip;
				m_ActiveClip = clip;
				return true;
			}
		}

		return false;
	}

	bool AnimationController::RemoveClip(const std::string& name)
	{
		auto& it = m_Clips.find(name);
		if (it != m_Clips.end())
		{
			if (it->second == m_ActiveClip)
				m_ActiveClip = nullptr;

			m_Clips.erase(name);
			return true;
		}

		return false;
	}

	bool AnimationController::SetActiveClip(const std::string& name)
	{
		auto& it = m_Clips.find(name);
		if (it != m_Clips.end())
		{
			m_ActiveClip = it->second;
			return true;
		}

		return false;
	}

	Ref<AnimationClip> AnimationController::GetActiveClip()
	{
		return m_ActiveClip;
	}

	const std::vector<glm::mat4>* AnimationController::GetJoints()
	{
		if (m_ActiveClip)
			return &m_ActiveClip->GetJoints();

		return nullptr;
	}

	void AnimationController::Update()
	{
		m_ActiveClip->Update();
	}

	void AnimationController::CopyJoints(std::vector<glm::mat4>& dist, uint32_t& out_index)
	{
		m_ActiveClip->CopyJoints(dist, out_index);
	}
}