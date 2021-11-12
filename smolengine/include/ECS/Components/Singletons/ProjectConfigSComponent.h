#pragma once
#pragma once

#include <string>
#include <unordered_map>

namespace cereal
{
	class access;
}

namespace SmolEngine
{
	struct SceneConfigData
	{
		std::string FilePath = "";
		std::string FileName = "";

	private:

		friend class cereal::access;

		template<typename Archive>
		void serialize(Archive& archive)
		{
			archive(FilePath, FileName);
		}
	};

	struct ProjectConfigSComponent
	{
		ProjectConfigSComponent();
		~ProjectConfigSComponent();

		inline static ProjectConfigSComponent*           Instance = nullptr;
		std::unordered_map<uint32_t, SceneConfigData>    m_Scenes;
		std::string                                      m_AssetFolder = "";

		static ProjectConfigSComponent* Get();

	private:

		friend class cereal::access;
		friend class BuildPanel;

		template<typename Archive>
		void serialize(Archive& archive)
		{
			archive(m_Scenes, m_AssetFolder);
		}
	};
}