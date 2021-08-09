#include "stdafx.h"
#include "Import/OzzImported.h"

#include <regex>

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	bool OzzImporter::ImportGltf(const std::string& filePath)
	{
		std::filesystem::path path(filePath);
		std::string skeleton_name = path.filename().stem().u8string() + "_skeleton.ozz";
		std::string anim_name = path.filename().stem().u8string() + "_animation.ozz";

		std::string config = R"( --config="{\"skeleton\":{\"filename\":\"skeleton_name.ozz\"},\"animations\":[{\"filename\":\"animation_name.ozz\"}]}")";

		config = std::regex_replace(config, std::regex("skeleton_name.ozz"), skeleton_name);
		config = std::regex_replace(config, std::regex("animation_name.ozz"), anim_name);

		std::string command_line = "gltf2ozz.exe --file=" + filePath + config;
		bool result = std::system(command_line.c_str()) == 0;
		if (result)
		{
			std::filesystem::rename(std::filesystem::current_path().u8string() + "/" + skeleton_name, path.parent_path().u8string() + "/" + skeleton_name);
			std::filesystem::rename(std::filesystem::current_path().u8string() + "/" + anim_name, path.parent_path().u8string() + "/" + anim_name);
		}

		return result;
	}
}