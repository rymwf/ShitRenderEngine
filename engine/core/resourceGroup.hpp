#pragma once
#include "prerequisites.hpp"
#include "archiveManager.hpp"

using resource_creator_signature_type =
	Resource *(
		std::string_view name,
		const ParameterMap *parameterMap,
		std::string_view group,
		bool manullyLoad,
		ManualResourceLoader *loader,
		size_t size,
		void const *data);

/**
 * @brief  TODO: load order list
 * 
 */
class ResourceGroup
{
	friend class ResourceGroupManager;

public:
	enum Status
	{
		UNINITIALISED,
		INITIALISING,
		INITIALISED,
		LOADING,
		LOADED,
	};

	ResourceGroup(ResourceGroupManager *creator, std::string_view name);
	~ResourceGroup();

	void addArchive(std::string_view name);
	void addArchive(Archive *archive);

	Resource *getResource(std::string_view name);

	bool containsResource(std::string_view name);

	bool containsFile(std::string_view name);

	/**
	 * @brief donot add to declare list
	 * 
	 * @param declaration 
	 * @return Resource* 
	 */
	Resource *createResource(const ResourceDeclaration &declaration, size_t size = 0, void const *data = nullptr);

	Resource *createOrRetrieveResource(const ResourceDeclaration &declaration, size_t size = 0, void const *data = nullptr);

	void addResource(Resource *resource);

	void removeResource(std::string_view name);

	void declareResource(const ResourceDeclaration &declaration);
	void undeclareResource(std::string_view name);

	void createDeclaredResources();
	void prepareDeclaredResources();
	void loadDeclaredResources();
	void unloadDeclaredResources();

	constexpr Status getStatus() const { return _status; }

private:
	std::string _name;
	std::vector<ResourceDeclaration> _declareList;

	//store all resource relative path, including archive name
	std::unordered_set<std::string> _resourcePath;
	std::unordered_set<std::string> _archives;

	std::unordered_map<std::string, std::unique_ptr<Resource>> _resources;

	Status _status;

	ResourceGroupManager *_creator;
};

class ResourceGroupManager : public Singleton<ResourceGroupManager>
{
	std::unordered_map<std::string, std::unique_ptr<ResourceGroup>> _resourceGroups;

	ResourceGroup *createResourceGroup(std::string_view name);

	std::unordered_map<ResourceType, std::function<resource_creator_signature_type>> _resourceCreators;

public:
	static std::string DEFAULT_GROUP_NAME;

	ResourceGroupManager();
	~ResourceGroupManager() {}

	ResourceGroup *createOrRetrieveResourceGroup(std::string_view group = DEFAULT_GROUP_NAME);

	ResourceGroup *getResourceGroup(std::string_view group = DEFAULT_GROUP_NAME) const;

	const std::function<resource_creator_signature_type> &getResourceCreator(ResourceType type) const;

	void registerResourceCreator(ResourceType type, const std::function<resource_creator_signature_type> &func);

	void openResource(std::string_view name, std::string_view group);
	void declareResource(const ResourceDeclaration &decl, std::string_view group);

	bool existGroup(std::string_view group = DEFAULT_GROUP_NAME) const;

	///**
	// * @brief parse script and create resources, not to load resource
	// *
	// * @param name
	// */
	//void initialiseResourceGroup(std::string_view name = DEFAULT_GROUP_NAME);
	//void initialiseAllResourceGroups();

	/**
	 * @brief unload all resources in the group, and remove all resources in their managers
	 * 
	 * @param name 
	 */
	void clearResourceGroup(std::string_view name = DEFAULT_GROUP_NAME);

	/**
	 * @brief clear first, and then remove the group
	 * 
	 * @param name 
	 */
	void destroyResourceGroup(std::string_view name = DEFAULT_GROUP_NAME);

	//void addResourceLocation(const std::string& name,const std::string&);
};
