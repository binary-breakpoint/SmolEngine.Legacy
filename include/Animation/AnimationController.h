#pragma once

#include "Animation/AnimationClip.h"

#include <unordered_map>

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	class AnimationController
	{
	public:
		bool                             AddClip(const AnimationClipCreateInfo& info, const std::string& name);
		bool                             RemoveClip(const std::string& name);
		bool                             SetActiveClip(const std::string& name);
		Ref<AnimationClip>               GetActiveClip();
						    
	private:			    
		void                             Update();
		void                             CopyJoints(std::vector<glm::mat4>& dist, uint32_t& out_index);

	private:
		Ref<AnimationClip>               m_ActiveClip = nullptr;
		std::unordered_map<std::string,
			Ref<AnimationClip>>          m_Clips;

		friend class DeferredRenderer;
	};
}