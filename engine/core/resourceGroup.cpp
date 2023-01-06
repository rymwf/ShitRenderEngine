#include "resourceGroup.hpp"
#include "archiveManager.hpp"
#include "resource.hpp"
#include <string_view>

ResourceGroup::ResourceGroup(ResourceGroupManager *creator, std::string_view name) : _creator(creator), _name(name)
{
}
ResourceGroup::~ResourceGroup() {}

void ResourceGroup::addArchive(std::string_view name)
{
	auto &&archiveManager = ArchiveManager::getSingleton();
	if (archiveManager.exists(name))
	{
		auto archive = archiveManager.getArchive(name);
		addArchive(archive);
	}
	else
		LOG("archive", name, "does not exist");
}
void ResourceGroup::addArchive(Archive *archive)
{
	auto &&archiveName = archive->getName();
	auto &&archivePath = archive->getPath();
	auto len = archivePath.size();

	std::vector<std::filesystem::path> items;
	archive->list(items, {}, true); //show all subitems
	for (auto &&e : items)
	{
		auto &&str = e.string().substr(len);
		std::string a;
		std::transform(str.cbegin(), str.cend(), std::back_inserter(a), [](auto c)
					   { return std::tolower(c); });
		_resourcePath.emplace(a);
	}
}
void ResourceGroup::addResource(Resource *resource)
{
	_resources.emplace(resource->getName(), std::unique_ptr<Resource>(resource));
}
void ResourceGroup::removeResource(std::string_view name)
{
	_resources.erase(std::string(name));
}
Resource *ResourceGroup::createResource(const ResourceDeclaration &declaration, size_t size, void const *data)
{
	auto res = _creator->getResourceCreator(declaration.type)(
		declaration.name,
		&declaration.parameters,
		_name,
		declaration.loader != nullptr,
		declaration.loader,
		size,
		data);
	addResource(res);
	return res;
}
Resource *ResourceGroup::createOrRetrieveResource(const ResourceDeclaration &declaration, size_t size, void const *data)
{
	if (containsResource(declaration.name))
		return getResource(declaration.name);
	return createResource(declaration, size, data);
}
void ResourceGroup::createDeclaredResources()
{
	for (auto &&e : _declareList)
		createResource(e);
}
Resource *ResourceGroup::getResource(std::string_view name)
{
	return _resources.at(std::string(name)).get();
}
bool ResourceGroup::containsResource(std::string_view name)
{
	return _resources.contains(std::string(name));
}
bool ResourceGroup::containsFile(std::string_view name)
{
	std::string_view archiveName, fileName;
	parseResourceName(name, archiveName, fileName);
	auto archivePath = ArchiveManager::getSingleton().getArchive(archiveName)->getPath();
	auto fullpath = std::string(archivePath) + "/" + std::string(fileName);
	return std::filesystem::exists(fullpath);
}
void ResourceGroup::declareResource(const ResourceDeclaration &declaration)
{
	if (containsFile(declaration.name))
	{
		_declareList.emplace_back(declaration);
	}
	else
	{
		LOG("file ", declaration.name, " does not exist!!! cannot declare it")
	}
}
void ResourceGroup::undeclareResource(std::string_view name)
{
	auto it = std::find_if(_declareList.cbegin(), _declareList.cend(), [&name](auto &&e)
						   { return e.name == name; });
	if (it != _declareList.cend())
		_declareList.erase(it);
}
void ResourceGroup::prepareDeclaredResources()
{
	for (auto &&e : _declareList)
		getResource(e.name)->prepare();
}
void ResourceGroup::loadDeclaredResources()
{
	for (auto &&e : _declareList)
		getResource(e.name)->load();
}
void ResourceGroup::unloadDeclaredResources()
{
	for (auto &&e : _declareList)
		getResource(e.name)->unload();
}
//void ResourceGroup::listFiles(std::vector<std::filesystem::path> &outputList,
//							  std::string_view file_extentions,
//							  bool forceRecursive)
//{

//}
//========================================
std::string ResourceGroupManager::DEFAULT_GROUP_NAME = "Default";
ResourceGroupManager::ResourceGroupManager()
{
}
ResourceGroup *ResourceGroupManager::createResourceGroup(std::string_view name)
{
	return _resourceGroups.emplace(name, std::make_unique<ResourceGroup>(this, name)).first->second.get();
}
ResourceGroup *ResourceGroupManager::getResourceGroup(std::string_view group) const
{
	return _resourceGroups.at(std::string(group)).get();
}
void ResourceGroupManager::registerResourceCreator(
	ResourceType type,
	const std::function<resource_creator_signature_type> &func)
{
	_resourceCreators.emplace(type, func);
}
const std::function<resource_creator_signature_type> &
ResourceGroupManager::getResourceCreator(ResourceType type) const
{
	if (_resourceCreators.contains(type))
		return _resourceCreators.at(type);
	THROW("failed to find resource creator, resource type", (uint32_t)type);
}
ResourceGroup *ResourceGroupManager::createOrRetrieveResourceGroup(std::string_view group)
{
	if (existGroup(group))
		return getResourceGroup(group);
	else
		return createResourceGroup(group);
}
bool ResourceGroupManager::existGroup(std::string_view group) const
{
	return _resourceGroups.contains(std::string(group));
}
void ResourceGroupManager::openResource(std::string_view name, std::string_view group)
{
}