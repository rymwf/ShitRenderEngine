#include "resourceManager.hpp"

ResourceManager::ResourceManager()
{
}

Resource *ResourceManager::create(
	std::string_view name,
	const ParameterMap *parameterMap,
	std::string_view group,
	bool manullyLoad,
	ManualResourceLoader *loader)
{
	auto ret = createImpl(name, parameterMap, group, manullyLoad, loader);
	add(ret);
	//_signal(ret, Event::RESOURCE_CREATED);
	return ret;
}
Resource *ResourceManager::createOrRetrieve(
	std::string_view name,
	const ParameterMap *parameterMap,
	std::string_view group,
	bool manullyLoad,
	ManualResourceLoader *loader)
{
	if (exists(name, group))
		return get(name, group);
	return create(name, parameterMap, group, manullyLoad, loader);
}
void ResourceManager::prepare(
	std::string_view name,
	std::string_view group,
	bool background)
{
	get(name, group)->prepare(background);
}
void ResourceManager::load(
	std::string_view name,
	std::string_view group,
	bool background)
{
	get(name, group)->load(background);
}
void ResourceManager::unload(
	std::string_view name,
	std::string_view group)
{
	get(name, group)->unload();
}
void ResourceManager::prepareGroup(std::string_view group, bool background)
{
	for (auto &&e : _groups[std::string(group)])
		e.second->prepare(background);
}
void ResourceManager::loadGroup(std::string_view group, bool background)
{
	for (auto &&e : _groups[std::string(group)])
		e.second->load(background);
}
void ResourceManager::unloadGroup(std::string_view group)
{
	for (auto &&e : _groups[std::string(group)])
		e.second->unload();
}