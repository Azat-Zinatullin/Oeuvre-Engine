#pragma once

#include <filesystem>
#include <vector>

namespace Oeuvre::FileSystem
{
	bool Exists(const std::filesystem::path& filepath);
	bool Exists(const std::string& path);
	bool IsDirectory(const std::filesystem::path& filepath);

	std::vector<std::filesystem::path> FindFilesRecursiveByExtension(const std::filesystem::path& directory, const std::string& extension);
	std::filesystem::path FindFileRecursiveByName(const std::filesystem::path& directory, const std::string& name);
}