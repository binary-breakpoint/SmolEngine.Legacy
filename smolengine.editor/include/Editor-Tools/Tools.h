#pragma once

#include <string>

namespace SmolEngine
{
	class Tools
	{
	public:

		static bool FileExtensionCheck(std::string& path, const std::string& ext);
		static bool FileBrowserDragAndDropCheck(const std::string& ext, std::string& out_path);
	};
}