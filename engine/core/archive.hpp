#pragma once
#include "prerequisites.hpp"
#include <filesystem>
#include <fstream>

class Archive
{

public:
	enum class FileType
	{
		REGULAR,
		DIRECTORY,
	};

	virtual ~Archive();

	//default name is the filename of the entry
	constexpr std::string_view getName() const
	{
		return _name;
	}
	constexpr std::string_view getPath() const { return _path; }

	constexpr ArchiveType getType() const { return _type; }

	virtual bool exists(std::string_view subItem) = 0;

	virtual bool isCaseSensitive() const = 0;

	/**
	 * @brief 
	 * 
	 * @param outputList 
	 * @param file_extention split by ,
	 * @param forceRecursive 
	 */
	virtual void list(std::vector<std::filesystem::path> &outputList,
					  std::string_view file_extentions = {},
					  bool forceRecursive = false) = 0;

	virtual std::shared_ptr<std::iostream> open(std::string_view relativePath,
												std::ios_base::openmode openmode = std::ios_base::in) = 0;

	virtual void close() = 0;
	//void refresh() { _path.refresh(); }
	//uintmax_t size() const { return _path.file_size(); }

protected:
	std::string _path;
	std::string _name;
	ArchiveType _type;
	bool _recursive{false};
	std::shared_ptr<std::iostream> _fstream{};

	Archive() = delete;
	Archive(std::string_view name, std::string_view fullpath, ArchiveType type, bool recursive);
};

class DirectoryArchive : public Archive
{
	void findFiles(
		std::string_view pattern,
		bool recursive,
		bool dirs,
		std::string &outputList);

public:
	DirectoryArchive(std::string_view name, std::string_view fullpath, bool recursive);
	~DirectoryArchive() { close(); }
	bool exists(std::string_view subItem) override;

	bool isCaseSensitive() const override { return false; }
	//void list(std::vector<std::filesystem::path> &outputList, std::string_view pattern , bool forceRecursive ) override;
	void list(std::vector<std::filesystem::path> &outputList, std::string_view file_extention, bool forceRecursive) override;

	std::shared_ptr<std::iostream> open(std::string_view relativePath,
										std::ios_base::openmode openmode = std::ios_base::in) override;
	void close() override;
};

class ZipArchive : public Archive
{
public:
	ZipArchive(std::string_view name, std::string_view fullpath, bool recursive);
	~ZipArchive() {}

	bool exists(std::string_view subItem) override { return false; }
	bool isCaseSensitive() const override { return false; }
	//void list(std::vector<std::filesystem::path> &outputList, std::string_view pattern , bool forceRecursive ) override {}
	void list(std::vector<std::filesystem::path> &outputList, std::string_view file_extention, bool forceRecursive) override {}

	std::shared_ptr<std::iostream> open(std::string_view relativePath,
										std::ios_base::openmode openmode = std::ios_base::in) override { return {}; }
	void close() override {}
};
