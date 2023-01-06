#include "resource.hpp"
#include "resourceManager.hpp"
#include "resourceGroup.hpp"

Resource::Resource(std::string_view name,
				   std::string_view groupName,
				   bool manuallyLoad,
				   ManualResourceLoader *loader)
	: _group(groupName),
	  _manuallyLoad(manuallyLoad),
	  _loader(loader)
{
	_name = name.empty() ? "gpuResource" + std::to_string(getId()) : name;
	parseResourceName(_name, _archiveName, _fileName);
}
Resource::~Resource()
{
}
void Resource::setLoadingState(LoadingState state)
{
	_loadingState = state;
	_cv.notify_all();
}
void Resource::prepareImpl()
{
	if (_data.empty())
	{
		auto a = open(std::ios::in);
		a->seekg(0, std::ios::end);
		auto size = a->tellg();
		a->seekg(0, std::ios::beg);
		_data.resize(size);
		a->read(_data.data(), size);
		close();
	}
}
void Resource::unprepareImpl()
{
	_data.clear();
}
void Resource::prepare(bool backgroundThread)
{
	if (_loadingState != LoadingState::UNLOADED && _loadingState != LoadingState::PREPARING)
		return;
	if (_loadingState == LoadingState::PREPARING)
	{
		auto lk = std::unique_lock(_mutex);
		_cv.wait(lk, [this]
				 { return this->_loadingState != Resource::LoadingState::PREPARING; });
		return;
	}
	//unloaded
	setLoadingState(LoadingState::PREPARING);
	LOG("preparing resource name:", _name, "group:", _group);

	if (_manuallyLoad)
	{
		if (_loader)
		{
			_loader->prepareResouce(this);
		}
		else
		{
			ST_LOG("manual resource loader is not provided");
		}
	}
	else
	{
		prepareImpl();
	}
	setLoadingState(LoadingState::PREPARED);
	_signal(this, Event::PREPARE_COMPLETE);
}
void Resource::load(bool backgroundThread)
{
	if (_loadingState == LoadingState::PREPARING)
	{
		auto lk = std::unique_lock(_mutex);
		_cv.wait(lk, [this]
				 { return this->_loadingState != Resource::LoadingState::PREPARING; });
	}
	if (_loadingState != LoadingState::UNLOADED && _loadingState != LoadingState::PREPARED && _loadingState != LoadingState::LOADING)
		return;

	if (_loadingState == LoadingState::LOADING)
	{
		auto lk = std::unique_lock(_mutex);
		_cv.wait(lk, [this]
				 { return this->_loadingState != Resource::LoadingState::LOADING; });
		if (_loadingState != LoadingState::LOADED)
			THROW("another thread failed in loading");
	}
	try
	{
		if (_loadingState == LoadingState::UNLOADED)
			prepareImpl();

		setLoadingState(LoadingState::LOADING);
		LOG("loading resource name:", _name, "group:", _group);

		preLoadImpl();
		if (_manuallyLoad)
		{
			if (_loader)
				_loader->loadResource(this);
			else
				LOG("manual loader is not provided");
		}
		else
		{
			loadImpl();
		}
		postLoadImpl();
	}
	catch (const std::exception &e)
	{
		LOG(e.what());
		unload();
		throw; //re throw
	}
	setLoadingState(LoadingState::LOADED);
	_signal(this, Event::LOAD_COMPLETE);
}
void Resource::reload(bool backgroundThread)
{
	unload();
	load(backgroundThread);
}
void Resource::unload()
{
	LOG("unloading resource name:", _name, "group:", _group);
	setLoadingState(LoadingState::UNLOADING);
	preUnloadImpl();
	unloadImpl();
	postUnloadImpl();
	setLoadingState(LoadingState::UNLOADED);
	_signal(this, Event::UNLOAD_COMPLETE);
}
std::string Resource::getFullPath() const
{
	auto archivePath = ArchiveManager::getSingleton().getArchive(_archiveName)->getPath();
	return std::string(archivePath) + "/" + std::string(_fileName);
}
std::shared_ptr<std::iostream> Resource::open(std::ios_base::openmode openmode)
{
	if (!ArchiveManager::getSingleton().exists(_archiveName))
		THROW("failed to find archvie", _archiveName)
	return ArchiveManager::getSingleton().getArchive(_archiveName)->open(_fileName, openmode);
}
void Resource::close()
{
	ArchiveManager::getSingleton().getArchive(_archiveName)->close();
}
void Resource::updateData(size_t offset, size_t size, void const *data)
{
	if (size + offset > _data.size())
		_data.resize(offset + size);
	memcpy(&_data[offset], data, size);
}
void Resource::save()
{
	auto ss = open(std::ios::out);
	// ss->seekp(0, std::ios::beg);
	ss->write(_data.data(), _data.size());
	if (ss->fail())
	{
		LOG("failed to write to file")
	}
	close();
}