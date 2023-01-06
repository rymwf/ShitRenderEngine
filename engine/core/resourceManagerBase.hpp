#pragma once
#include "prerequisites.hpp"
#include "idObject.hpp"

template <typename T>
class ResourceManagerBase
{
public:
	inline static std::string DEFAULT_GROUP_NAME = "Default";
	ResourceManagerBase() {}
	virtual ~ResourceManagerBase() {}

	std::unordered_map<std::string, std::unique_ptr<T>> *createGroup(std::string_view group = DEFAULT_GROUP_NAME);
	std::unordered_map<std::string, std::unique_ptr<T>> *createOrRetrieveGroup(std::string_view group = DEFAULT_GROUP_NAME);

	void add(T *resource);

	bool exists(std::string_view name, std::string_view group = DEFAULT_GROUP_NAME) const;
	bool groupExists(std::string_view group = DEFAULT_GROUP_NAME) const;

	void remove(IdType id);
	void remove(std::string_view name, std::string_view group = DEFAULT_GROUP_NAME);
	void remove(const T *res);
	void removeGroup(std::string_view group = DEFAULT_GROUP_NAME);
	void removeAll();

	T *get(std::string_view name, std::string_view group = DEFAULT_GROUP_NAME) const
	{
		return _groups.at(std::string(group)).at(std::string(name)).get();
	}
	T *get(IdType id) const { return _idMap.at(id); }

	//std::unordered_map<std::string, std::unique_ptr<T>> *getGroup(std::string_view group = DEFAULT_GROUP_NAME) const
	//{
	//	return &_groups.at(std::string(group));
	//}

protected:
	std::unordered_map<std::string,
					   std::unordered_map<std::string, std::unique_ptr<T>>>
		_groups;
	std::unordered_map<IdType, T *> _idMap;

	virtual void removeImpl(const T *res) {}
	virtual void removeGroupImpl(std::string_view group) {}
	virtual void removeAllImpl() {}
	virtual void createGroupImpl(std::string_view group) {}
};

template <typename T>
std::unordered_map<std::string, std::unique_ptr<T>> *
ResourceManagerBase<T>::createGroup(std::string_view group)
{
	auto ret = &_groups.emplace(group, std::unordered_map<std::string, std::unique_ptr<T>>()).first->second;
	createGroupImpl(group);
	return ret;
}
template <typename T>
std::unordered_map<std::string, std::unique_ptr<T>> *
ResourceManagerBase<T>::createOrRetrieveGroup(std::string_view group)
{
	std::string groupName = group;
	auto it = _groups.find(groupName);
	if (it == _groups.cend())
		return createGroup(groupName);
	return &it->second;
}
template <typename T>
void ResourceManagerBase<T>::add(T *resource)
{
	_groups[std::string(resource->getGroupName())].emplace(resource->getName(), std::unique_ptr<T>(resource));
	_idMap.emplace(resource->getId(), resource);
}
template <typename T>
void ResourceManagerBase<T>::remove(IdType id)
{
	auto a = _idMap[id];
	removeImpl(a);
	_groups[a->getGroup()].erase(a->getName());
	_idMap.erase(id);
}
template <typename T>
void ResourceManagerBase<T>::remove(std::string_view name, std::string_view group)
{
	auto ele = _groups[group][name].get();
	removeImpl(ele);
	_idMap.erase(ele->getId());
	_groups[group].erase(name);
}
template <typename T>
void ResourceManagerBase<T>::remove(const T *res)
{
	removeImpl(res);
	_idMap.erase(res->getId());
	_groups[std::string(res->getGroupName())].erase(std::string(res->getName()));
}
template <typename T>
void ResourceManagerBase<T>::removeGroup(std::string_view group)
{
	std::string str = group;
	for (auto &&e : _groups[str])
	{
		_idMap.erase(e.second->getId());
	}
	_groups.erase(str);
	removeGroupImpl(group);
}
template <typename T>
void ResourceManagerBase<T>::removeAll()
{
	_groups.clear();
	_idMap.clear();
	removeAllImpl();
}
template <typename T>
bool ResourceManagerBase<T>::groupExists(std::string_view group) const
{
	return _groups.find(std::string(group)) != _groups.cend();
}
template <typename T>
bool ResourceManagerBase<T>::exists(std::string_view name, std::string_view group) const
{
	std::string str{group};
	return _groups.contains(str) &&
		   _groups.at(str).contains(std::string(name));
}