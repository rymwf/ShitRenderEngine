#include "gpuResource.hpp"
#include "root.hpp"

GPUResource::GPUResource(GPUResourceManager *creator,
						 std::string_view group,
						 bool frameDependent)
	: _creator(creator), _group(group)
{
	_name = "gpuResource" + std::to_string(getId());
	frameDependent ? setGPUResourceCount(Root::getSingleton().getScreen()->getSwapchain()->GetImageCount())
				   : setGPUResourceCount(1);
}
GPUResource::GPUResource(GPUResourceManager *creator,
						 std::string_view name,
						 std::string_view group,
						 bool frameDependent)
	: _creator(creator), _name(name), _group(group)
{
	frameDependent ? setGPUResourceCount(Root::getSingleton().getScreen()->getSwapchain()->GetImageCount())
				   : setGPUResourceCount(1);
}
void GPUResource::setGPUResourceCount(uint32_t count)
{
	_gpuResourceCount = count;
	_statuses.resize(_gpuResourceCount, Status::DESTROYED);
	setGPUResourceCountImpl(count);
}
void GPUResource::prepare(int index)
{
	if (index < 0)
	{
		for (int i = 0; i < _gpuResourceCount; ++i)
		{
			if (_statuses[i] == Status::DESTROYED)
			{
				prepareImpl(i);
				_statuses[i] = Status::PREPARED;
			}
		}
	}
	else
	{
		index = (std::min)(_gpuResourceCount - 1, (uint32_t)index);
		if (_statuses[index] == Status::DESTROYED)
		{
			prepareImpl(index);
			_statuses[index] = Status::PREPARED;
		}
	}
}
void GPUResource::upload(int index)
{
	if (index < 0)
	{
		for (int i = 0; i < _gpuResourceCount; ++i)
		{
			if (_statuses[i] == Status::DESTROYED)
			{
				prepareImpl(i);
				_statuses[i] = Status::PREPARED;
			}

			if (_statuses[i] == Status::PREPARED)
			{
				// LOG("uploading gpuresource name:", _name, "group:", _group);
				uploadImpl(i);
				//_uploadSignal(this);
				_statuses[i] = Status::UPLOADED;
			}
		}
	}
	else
	{
		index = (std::min)(_gpuResourceCount - 1, (uint32_t)index);
		if (_statuses[index] == Status::DESTROYED)
		{
			prepareImpl(index);
			_statuses[index] = Status::PREPARED;
		}
		if (_statuses[index] == Status::PREPARED)
		{
			// LOG("uploading gpuresource name:", _name, "group:", _group);
			uploadImpl(index);
			_statuses[index] = Status::UPLOADED;
			//_uploadSignal(this);
		}
	}
}
void GPUResource::destroy(int index)
{
	if (index < 0)
	{
		for (int i = 0; i < _gpuResourceCount; ++i)
		{
			if (_statuses[i] != Status::DESTROYED)
			{
				destroyImpl(i);
				_statuses[i] = Status::DESTROYED;
			}
		}
	}
	else if ((uint32_t)index < _gpuResourceCount)
	{
		if (_statuses[index] != Status::DESTROYED)
		{
			destroyImpl(index);
			_statuses[index] = Status::DESTROYED;
		}
	}
	else
	{
		THROW("index out of range, index=", index);
	}
}
//======================================================
GPUResourceManager::GPUResourceManager()
{
}
std::span<GPUResource *> GPUResourceManager::createGroup(std::string_view group)
{
	auto ret = _groups.emplace(group, std::vector<GPUResource *>{}).first->second;
	createGroupImpl(group);
	return ret;
}
std::span<GPUResource *> GPUResourceManager::createOrRetrieveGroup(std::string_view group)
{
	std::string groupName(group);
	auto it = _groups.find(groupName);
	if (it == _groups.cend())
		return createGroup(groupName);
	return it->second;
}
void GPUResourceManager::add(GPUResource *resource)
{
	_idMap.emplace(resource->getId(), std::unique_ptr<GPUResource>(resource));
	_groups[std::string(resource->getGroupName())].emplace_back(resource);
}
bool GPUResourceManager::exists(IdType id) const
{
	return _idMap.contains(id);
}
bool GPUResourceManager::groupExists(std::string_view group) const
{
	return _groups.contains(std::string(group));
}
void GPUResourceManager::remove(IdType id)
{
	if (exists(id))
		remove(_idMap[id].get());
}
void GPUResourceManager::remove(const GPUResource *res)
{
	std::erase(_groups[std::string(res->getGroupName())], res);
	removeImpl(res);
	_idMap.erase(res->getId());
}
void GPUResourceManager::removeGroup(std::string_view group)
{
	removeGroupImpl(group);
	std::string str(group);
	for (auto &&e : _groups[str])
	{
		_idMap.erase(e->getId());
	}
	_groups.erase(str);
}
void GPUResourceManager::removeAll()
{
	removeAllImpl();
	_groups.clear();
	_idMap.clear();
}

void GPUResourceManager::removeImpl(const GPUResource *res)
{
	const_cast<GPUResource *>(res)->destroy();
}
void GPUResourceManager::upload(IdType id, int frameIndex)
{
	upload(get(id), frameIndex);
}
void GPUResourceManager::upload(GPUResource *resource, int frameIndex)
{
	resource->upload(frameIndex);
}
void GPUResourceManager::uploadGroup(std::string_view group)
{
	for (auto e : _groups.at(std::string(group)))
		e->upload();
}
void GPUResourceManager::uploadAll()
{
	for (auto &&group : _groups)
		for (auto e : group.second)
			e->upload();
}
void GPUResourceManager::removeFromGPU(IdType id)
{
	removeFromGPU(get(id));
}
void GPUResourceManager::removeFromGPU(GPUResource *resource)
{
	resource->destroy();
}
void GPUResourceManager::removeGroupFromGPU(std::string_view group)
{
	for (auto e : _groups[std::string(group)])
		e->destroy();
}
void GPUResourceManager::removeAllFromGPU()
{
	for (auto &&group : _groups)
	{
		for (auto &&e : group.second)
			e->destroy();
	}
}