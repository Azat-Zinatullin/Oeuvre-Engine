#include "ovpch.h"
#include "FileSystem.h"

namespace Oeuvre::FileSystem
{
	bool Exists(const std::filesystem::path& filepath)
	{
		return std::filesystem::exists(filepath);
	}

	bool Exists(const std::string& filepath)
	{
		return std::filesystem::exists(std::filesystem::path(filepath));
	}

	bool IsDirectory(const std::filesystem::path& filepath)
	{
		return std::filesystem::is_directory(filepath);
	}

	std::vector<std::filesystem::path> FindFilesRecursiveByExtension(const std::filesystem::path& directory, const std::string& extension)
	{
		std::vector<std::filesystem::path> foundFiles;
		try {
			for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
				if (entry.is_regular_file() && entry.path().extension() == extension) {
					foundFiles.push_back(entry.path());
				}
			}
		}
		catch (const std::filesystem::filesystem_error& e) {
			std::cerr << "Filesystem error: " << e.what() << std::endl;
		}
		return foundFiles;
	}

	std::filesystem::path FindFileRecursiveByName(const std::filesystem::path& directory, const std::string& name)
	{
		std::filesystem::path path = "";
		try {
			for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
				if (entry.is_regular_file() && entry.path().filename() == name) {
					path = entry.path();
					break;
				}
			}
		}
		catch (const std::filesystem::filesystem_error& e) {
			std::cerr << "Filesystem error: " << e.what() << std::endl;
		}
		return path;
	}
}
