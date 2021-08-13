#include "stdafx.h"
#include "Import/OzzImported.h"
#include <regex>

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	bool OzzImporter::ImportGltf(const std::string& filePath, const std::string& exePath)
	{
		std::filesystem::path path(filePath);
		std::string parent_path = path.parent_path().u8string();
		std::string command_line = exePath + "gltf2ozz.exe --file=" + filePath;
		bool ex = std::system(command_line.c_str()) == 0;
		{
			std::vector<std::string> new_files;
			for (auto& dir_entry : std::filesystem::directory_iterator(std::filesystem::current_path()))
			{
				if (dir_entry.path().extension().filename().u8string() == ".ozz")
					new_files.emplace_back(dir_entry.path().u8string());
			}

			for (auto& file : new_files)
			{
				std::filesystem::path p(file);
				std::filesystem::rename(file, parent_path + "/" + path.filename().stem().u8string() + "_" + p.filename().u8string());
			}
		}

		return ex;
	}
}