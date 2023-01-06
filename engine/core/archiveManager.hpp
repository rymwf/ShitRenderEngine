#pragma once
#include "archive.hpp"
#include "noncopyable.hpp"
#include "singleton.hpp"

std::string getShaderArchiveName();

/**
 * @brief load folder directory or zip file etc.
 * 
 */
class ArchiveManager : public NonCopyable, public Singleton<ArchiveManager>
{
	Archive *createArchive(std::string_view name,
						   std::string_view path,
						   ArchiveType type,
						   bool recursive = true);

public:
	using ArchiveGroup = std::unordered_map<std::string, std::unique_ptr<Archive>>;

	ArchiveManager();

	bool exists(std::string_view filename);

	Archive *createOrRetrieve(
		std::string_view name,
		std::string_view path,
		ArchiveType type,
		bool recursive = true);
	/**
	 * @brief Get the Archive object
	 * 
	 * @param archiveName only need the file name of archive
	 * @return Archive* 
	 */
	Archive *getArchive(std::string_view archiveName) const;

private:
	ArchiveGroup _archives;
};
