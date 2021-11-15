#pragma once

#ifndef FROSTIUM_SMOLENGINE_IMPL
#define FROSTIUM_SMOLENGINE_IMPL
#endif
#include <frostium/Animation/AnimationController.h>
#include <string>
#include <vector>

namespace SmolEngine
{
	class AnimationPanel
	{
	public:
		void                     Update();
		void                     Reset();
		void                     Open(const std::string& filePath, bool is_new);

	private:
		int                      m_ItemIndex = 0;
		AnimationClipCreateInfo  m_CreateInfo = {};
		std::string              m_CurrentPath = "";
		std::string              m_SearchBuffer = "";
		std::vector<std::string> m_Files;
	};
}