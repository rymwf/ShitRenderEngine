#include "archiveManager.hpp"
#include "root.hpp"

//===================================================
std::string getShaderArchiveName()
{
	std::string archiveName;
	auto rv = Root::getSingleton().getRendererVersion() & Shit::RendererVersion::TypeBitmask;
	switch (rv)
	{
	case Shit::RendererVersion::GL:
		archiveName = "shaders_spv_gl";
		break;
	case Shit::RendererVersion::VULKAN:
		archiveName = "shaders_spv_vk";
		break;
	}
	return archiveName;
}

ArchiveManager::ArchiveManager()
{
}
Archive *ArchiveManager::createArchive(
	std::string_view name,
	std::string_view path,
	ArchiveType type,
	bool recursive)
{
	Archive *arch;
	switch (type)
	{
	case ArchiveType::DIRECTORY:
		FALLTHROUGH;
	default:
		arch = new DirectoryArchive(name, path, recursive);
		break;
	case ArchiveType::ZIP:
		arch = new ZipArchive(name, path, recursive);
		break;
	}
	return _archives.emplace(arch->getName(), std::unique_ptr<Archive>(arch)).first->second.get();
}
bool ArchiveManager::exists(std::string_view filename)
{
	return _archives.contains(std::string(filename));
}
Archive *ArchiveManager::getArchive(std::string_view archiveName) const
{
	return _archives.at(std::string(archiveName)).get();
}
Archive *ArchiveManager::createOrRetrieve(
	std::string_view name,
	std::string_view path,
	ArchiveType type,
	bool recursive)
{
	auto it = _archives.find(std::string(name));
	if (it != _archives.cend())
		return it->second.get();
	return createArchive(name, path, type, recursive);
}