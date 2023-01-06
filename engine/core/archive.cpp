#include "archive.hpp"
Archive::Archive(std::string_view name, std::string_view path, ArchiveType type, bool recursive)
	: _name(name), _type(type), _recursive(recursive), _path(path)
{
	//auto it = path.crbegin();
	//auto end = path.crend();
	//while (it != end && (std::isspace(*it) || *it == '\\' || *it == '/'))
	//	++it;
	//_path = path.substr(0, end - it);
	//auto &&str = std::filesystem::path(_path).filename().string();
	//_name.resize(str.size());
	//std::transform(str.cbegin(), str.cend(), _name.begin(), [](auto c)
	//			   { return std::tolower(c); });
}
Archive::~Archive() {}
//================================
DirectoryArchive::DirectoryArchive(std::string_view name, std::string_view fullpath, bool recursive)
	: Archive(name, fullpath, ArchiveType::DIRECTORY, recursive)
{
	if (!std::filesystem::exists(fullpath))
	{
		//if folder donot exist, create a new one
		std::filesystem::create_directory(fullpath);
	}
}
bool DirectoryArchive::exists(std::string_view subItem)
{
	auto path = _path;
	path += "/";
	path += subItem;
	return std::filesystem::exists(path);
}

template <typename Iter>
void listHelper(
	std::vector<std::filesystem::path> &outputList,
	const std::vector<std::string_view> &extensions,
	Iter it)
{
	for (auto &&e : it)
	{
		auto p = e.path();
		auto tt = std::filesystem::status(p).type();
		if (tt == std::filesystem::file_type::regular)
		{
			if (extensions.empty())
				outputList.emplace_back(p);
			else
			{
				auto &&str = p.extension().string();
				std::string extension;
				std::transform(str.cbegin(), str.cend(), std::back_inserter(extension), [](auto c)
							   { return std::tolower(c); });
				for (auto &&ext : extensions)
				{
					if (ext == extension)
					{
						outputList.emplace_back(p);
						break;
					}
				}
			}
		}
	}
}
void DirectoryArchive::list(std::vector<std::filesystem::path> &outputList,
							std::string_view file_extensions,
							bool forceRecursive)
{
	outputList.clear();
	std::string a;
	std::transform(file_extensions.cbegin(), file_extensions.cend(), std::back_inserter(a), [](auto c)
				   { return std::tolower(c); });
	auto exts = split(a, ", ");
	if (forceRecursive || _recursive)
	{
		auto &&it = std::filesystem::recursive_directory_iterator(_path);
		listHelper(outputList, exts, it);
	}
	else
	{
		auto &&it = std::filesystem::directory_iterator(_path);
		listHelper(outputList, exts, it);
	}
}
std::shared_ptr<std::iostream> DirectoryArchive::open(std::string_view relativePath,
													  std::ios_base::openmode openmode)
{
	auto filename = _path + "/" + std::string(relativePath);
	auto ss= std::make_shared<std::fstream>(filename, openmode);
	if (!ss->is_open())
	{
		THROW("failed to open file", filename);
		return nullptr;
	}
	_fstream = std::static_pointer_cast<std::iostream>(ss);
	return _fstream;
}
void DirectoryArchive::close()
{
	auto p = std::dynamic_pointer_cast<std::fstream>(_fstream);
	if (p && p->is_open())
		p->close();
}
//============================================
ZipArchive::ZipArchive(std::string_view name, std::string_view fullpath, bool recursive)
	: Archive(name, fullpath, ArchiveType::ZIP, recursive)
{
	if (!std::filesystem::exists(fullpath))
	{
		LOG("zip archive", fullpath, " do not exist");
	}
}