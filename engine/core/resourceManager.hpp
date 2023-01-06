#pragma once
#include "resourceManagerBase.hpp"
#include "resource.hpp"

/**
 * @brief   group name is the same as ResourceGroupManager's
 * 
 */
class ResourceManager : public ResourceManagerBase<Resource>
{
public:
	ResourceManager();
	virtual ~ResourceManager() {}

	static inline std::string DEFAULT_GROUP_NAME = DEFAULT_GROUP_NAME;

	/**
	 * @brief 
	 * 
	 * @param name 
	 * @param group group must be the same as file group int resourceGroupManager
	 * @param manullyLoad 
	 * @param loader 
	 * @param parameterMap 
	 * @return Resource* 
	 */
	Resource *create(
		std::string_view name,
		const ParameterMap *parameterMap = nullptr,
		std::string_view group = DEFAULT_GROUP_NAME,
		bool manullyLoad = false,
		ManualResourceLoader *loader = nullptr);

	Resource *createOrRetrieve(
		std::string_view name,
		const ParameterMap *parameterMap = nullptr,
		std::string_view group = DEFAULT_GROUP_NAME,
		bool manullyLoad = false,
		ManualResourceLoader *loader = nullptr);

	void prepare(
		std::string_view name,
		std::string_view group = DEFAULT_GROUP_NAME,
		bool background = false);

	void load(
		std::string_view name,
		std::string_view group = DEFAULT_GROUP_NAME,
		bool background = false);

	void unload(
		std::string_view name,
		std::string_view group = DEFAULT_GROUP_NAME);

	void prepareGroup(std::string_view group = DEFAULT_GROUP_NAME, bool background = false);
	void loadGroup(std::string_view group = DEFAULT_GROUP_NAME, bool background = false);
	void unloadGroup(std::string_view group = DEFAULT_GROUP_NAME);

protected:
	virtual Resource *createImpl(
		std::string_view name,
		const ParameterMap *parameterMap,
		std::string_view group,
		bool manullyLoad,
		ManualResourceLoader *loader) = 0;
};