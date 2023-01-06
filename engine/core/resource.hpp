#pragma once
#include "prerequisites.hpp"
#include "idObject.hpp"
#include "singleton.hpp"

class ManualResourceLoader
{
public:
	virtual ~ManualResourceLoader() {}
	virtual void prepareResouce(Resource *resource) {}
	virtual void loadResource(Resource *resource) = 0;
};

class Resource : public IdObject<Resource>
{
public:
	enum class Event
	{
		LOAD_COMPLETE,
		PREPARE_COMPLETE,
		UNLOAD_COMPLETE
	};
	/// Enum identifying the loading state of the resource
	enum class LoadingState
	{
		/// Not loaded
		UNLOADED,
		/// Loading is in progress
		LOADING,
		/// Fully loaded
		LOADED,
		/// Currently unloading
		UNLOADING,
		/// Fully prepared
		PREPARED,
		/// Preparing is in progress
		PREPARING,
		/// Unloaded and marked for reload
		UNLOADED_MARKED_FOR_RELOAD,
		////
		//UPLOAED_TO_GPU,
		////
		//REMOVED_FROM_GPU,
	};
	Resource(
		std::string_view name,
		std::string_view groupName,
		bool manuallyLoad = false,
		ManualResourceLoader *loader = nullptr);
	virtual ~Resource();

	void prepare(bool backgroundThread = false);

	/**
	 * @brief load resource to memory
	 * 
	 * @param backgroundThread 
	 */
	void load(bool backgroundThread = false);

	void reload(bool backgroundThread = false);

	void unload();

	constexpr bool isReloadable() const { return !_manuallyLoad; }

	constexpr bool isManuallyLoad() const { return _manuallyLoad; }

	constexpr void setManuallyLoad(bool manuallyLoad) { _manuallyLoad = manuallyLoad; }

	constexpr bool isBackgroundLoad() const { return _backgroundLoad; }

	constexpr void setBackgroundLoad(bool backgroundLoad) { _backgroundLoad = backgroundLoad; }

	constexpr size_t getSize() const { return _data.size(); }

	constexpr std::span<char const> getData() const { return _data; }

	constexpr std::string_view getName() const { return _name; }

	constexpr std::string_view getGroupName() const { return _group; }

	std::string getFullPath() const;

	std::shared_ptr<std::iostream> open(std::ios_base::openmode openmode);

	void close();

	constexpr LoadingState getLoadingState() const
	{
		return _loadingState;
	}

	void setLoadingState(LoadingState state);

	void updateData(size_t offset, size_t size, void const *data);
	
	virtual void save();

protected:
	Shit::Signal<void(const Resource *, Resource::Event)> _signal;

	//constructed by archivename +'/'+filename
	std::string _name;
	std::string_view _archiveName;
	std::string_view _fileName;

	std::string _group;

	bool _backgroundLoad{false};
	bool _manuallyLoad{false};

	std::vector<char> _data;

	LoadingState _loadingState{};

	std::mutex _mutex;
	std::condition_variable _cv;

	ManualResourceLoader *_loader;

	virtual void prepareImpl();
	virtual void preLoadImpl() {}
	virtual void loadImpl() {}
	virtual void postLoadImpl() {}

	virtual void unprepareImpl();
	virtual void preUnloadImpl() {}
	virtual void unloadImpl() {}
	virtual void postUnloadImpl() {}
};
