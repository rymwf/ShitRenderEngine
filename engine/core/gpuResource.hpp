#pragma once
#include "resourceManagerBase.hpp"
#include "noncopyable.hpp"

class GPUResourceManager;
class GPUResource : virtual public IdObject<GPUResource>, public NonCopyable
{
public:
	enum class Status
	{
		//CREATED,
		DESTROYED, // destroy gpu memory
		PREPARED,  // allocated gpu memory, but data not update
		UPLOADED,  // gpu memory updated
	};
	GPUResource(GPUResourceManager *creator,
				std::string_view group,
				bool frameDependent = false);

	GPUResource(GPUResourceManager *creator,
				std::string_view name,
				std::string_view group,
				bool frameDependent = false);
	virtual ~GPUResource()
	{
		destroy();
	}

	virtual void prepare(int index = -1);
	/**
	 * @brief upload to gpu
	 *
	 * @param index  -1 mean upload all
	 */
	virtual void upload(int index = -1);
	virtual void destroy(int index = -1);
	// void clear();

	/**
	 * @brief
	 *
	 * @param name include archivename add filename
	 */
	virtual void saveToDisk(std::string_view name)
	{
		LOG("not implemented yet");
	}

	constexpr std::string_view getName() const { return _name; }
	constexpr std::string_view getGroupName() const { return _group; }
	constexpr GPUResourceManager *getCreator() const { return _creator; }

	// virtual size_t size() const = 0;
	constexpr Status getStatus(uint32_t index) const { return _statuses.at(index); }
	void setStatus(uint32_t index, Status status)
	{
		_statuses[index] = status;
		_statusChangeSignal(status, index);
	}
	void setAllStatus(Status status)
	{
		ranges::fill(_statuses, status);
		_statusChangeSignal(status, -1);
	}

	void setGPUResourceCount(uint32_t count);

	void addStatusListener(std::function<void(Status, int)> func)
	{
		if (!_statusChangeSignal.Contains(func))
			_statusChangeSignal.Connect(func);
	}
	void removeStatusListener(std::function<void(Status, int)> func)
	{
		_statusChangeSignal.Disconnect(func);
	}

protected:
	virtual void prepareImpl(int) {}
	virtual void uploadImpl(int) {}
	virtual void destroyImpl(int) {}
	virtual void setGPUResourceCountImpl(uint32_t){};

	std::string _name;
	std::string _group;
	GPUResourceManager *_creator;
	Shit::Signal<void(GPUResource::Status, int)> _statusChangeSignal;
	uint32_t _gpuResourceCount = 1;
	mutable std::vector<Status> _statuses;
};

class GPUResourceManager
{
public:
	using group_type = std::vector<GPUResource *>;
	using group_container_type = std::unordered_map<std::string, group_type>;
	using container_type = std::unordered_map<IdType, std::unique_ptr<GPUResource>>;

	inline static std::string DEFAULT_GROUP_NAME = "Default";

	enum class Event
	{
	};

	GPUResourceManager();
	virtual ~GPUResourceManager() {}

	std::span<GPUResource *> createGroup(std::string_view group = DEFAULT_GROUP_NAME);
	std::span<GPUResource *> createOrRetrieveGroup(std::string_view group = DEFAULT_GROUP_NAME);

	void add(GPUResource *resource);

	bool exists(IdType id) const;
	bool groupExists(std::string_view group = DEFAULT_GROUP_NAME) const;

	void remove(IdType id);
	void remove(const GPUResource *res);
	void removeGroup(std::string_view group = DEFAULT_GROUP_NAME);
	void removeAll();

	GPUResource *get(IdType id) const { return _idMap.at(id).get(); }

	void upload(IdType id, int frameIndex = -1);
	void upload(GPUResource *resource, int frameIndex = -1);
	void uploadGroup(std::string_view group = DEFAULT_GROUP_NAME);
	void uploadAll();

	void removeFromGPU(IdType id);
	void removeFromGPU(GPUResource *resource);
	void removeGroupFromGPU(std::string_view group = DEFAULT_GROUP_NAME);
	void removeAllFromGPU();

protected:
	group_container_type _groups;
	container_type _idMap;

	virtual void removeImpl(const GPUResource *res);
	virtual void removeGroupImpl(std::string_view group) {}
	virtual void removeAllImpl() {}
	virtual void createGroupImpl(std::string_view group) {}
};