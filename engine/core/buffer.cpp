#include "buffer.hpp"
#include "root.hpp"

Buffer::Buffer(GPUResourceManager *creator,
			   std::string_view groupName,
			   Shit::BufferUsageFlagBits usage,
			   Shit::MemoryPropertyFlagBits memoryProperty,
			   bool frameDependent,
			   size_t offsetAlignment)
	: _usage(usage), _memoryProperty(memoryProperty), _offsetAlignment(offsetAlignment),
	  GPUResource(creator, {}, groupName, frameDependent)
{
}
Buffer::~Buffer()
{
}
BufferView Buffer::allocate(size_t stride, size_t count, void const *data)
{
	if (count == 0)
		THROW("count should not be 0")
	destroy();

	auto offset = _data.size();
	auto l = stride * count;
	//static size_t baseAlignment = 16;
	//auto requestSize = (l / baseAlignment + (l % baseAlignment ? 1 : 0)) * baseAlignment;
	auto requestSize = (l / _offsetAlignment + (l % _offsetAlignment ? 1 : 0)) * _offsetAlignment;

	//_data.reserve(offset + requestSize);
	_data.resize(offset + requestSize);
	if (data)
		memcpy(&_data[offset], data, l);
	return BufferView{this, offset, stride, count};
}
BufferView Buffer::allocate(size_t stride, size_t count, int val)
{
	if (count == 0)
		THROW("count should not be 0")
	destroy();
	auto offset = _data.size();
	auto l = stride * count;
	// static size_t baseAlignment = 16;
	// auto requestSize = (l / baseAlignment + (l % baseAlignment ? 1 : 0)) * baseAlignment;
	auto requestSize = (l / _offsetAlignment + (l % _offsetAlignment ? 1 : 0)) * _offsetAlignment;

	//_data.reserve(offset + (std::max)(_offsetAlignment, requestSize));
	_data.resize(offset + requestSize);

	memset(&_data[offset], val, l);
	return BufferView{this, offset, stride, count};
}
void Buffer::setData(size_t offset, size_t size, void const *data)
{
	if (offset + size > _data.size())
		THROW("out of buffer range");
	for (uint32_t i = 0; i < _gpuResourceCount; ++i)
	{
		if (_statuses[i] == Status::UPLOADED)
			setStatus(i, Status::PREPARED);
	}
	if (data)
		memcpy(&_data[offset], data, size);
}
void Buffer::setData(size_t offset, size_t size, int val)
{
	if (offset + size > _data.size())
		THROW("out of buffer range");
	memset(&_data[offset], val, size);
	if (_statuses[0] == Status::UPLOADED)
		setAllStatus(Status::PREPARED);
}
void Buffer::prepareImpl(int index)
{
	if (_data.empty())
	{
		LOG("buffer is empty")
		return;
	}
	_buffers.resize(_gpuResourceCount);
	if (!_buffers[index])
		_buffers[index] = Root::getSingleton().getDevice()->Create(
			Shit::BufferCreateInfo{{}, _data.size(), _usage, _memoryProperty}, _data.data());
}
void Buffer::uploadImpl(int index)
{
	if (_data.empty())
		return;
	auto device = Root::getSingleton().getDevice();
	if (!bool(_memoryProperty & Shit::MemoryPropertyFlagBits::HOST_VISIBLE_BIT))
	{
		if (!_stageBuffer)
		{
			// create stageBuffer
			_stageBuffer = device->Create(
				Shit::BufferCreateInfo{
					{},
					_data.size(),
					Shit::BufferUsageFlagBits::TRANSFER_SRC_BIT,
					Shit::MemoryPropertyFlagBits::HOST_COHERENT_BIT |
						Shit::MemoryPropertyFlagBits::HOST_VISIBLE_BIT},
				_data.data());
		}
		else
		{
			void *data;
			_stageBuffer->MapMemory(0, _data.size(), &data);
			memcpy(data, _data.data(), _data.size());
			_stageBuffer->UnMapMemory();
		}

		// transfer data
		Root::getSingleton().executeOneTimeCommand(
			[&](Shit::CommandBuffer *cmdBuffer)
			{
				std::vector<Shit::BufferMemoryBarrier> barriers{
					{Shit::AccessFlagBits::SHADER_READ_BIT,
					 Shit::AccessFlagBits::TRANSFER_WRITE_BIT,
					 ST_QUEUE_FAMILY_IGNORED,
					 ST_QUEUE_FAMILY_IGNORED,
					// Root::getSingleton().getGraphicsQueue()->GetFamilyIndex(),
					// Root::getSingleton().getTransferQueue()->GetFamilyIndex(),
					 _buffers[index],
					 0,
					 _buffers[index]->GetCreateInfoPtr()->size},
				};
				cmdBuffer->PipelineBarrier(
					Shit::PipelineBarrierInfo{
						Shit::PipelineStageFlagBits::FRAGMENT_SHADER_BIT,
						Shit::PipelineStageFlagBits::TRANSFER_BIT,
						{},
						0,
						0,
						(uint32_t)barriers.size(),
						barriers.data(),
					});

				Shit::BufferCopy copyregion{
					0, 0, (uint32_t)_data.size()};
				cmdBuffer->CopyBuffer(Shit::CopyBufferInfo{
					_stageBuffer,
					_buffers[index],
					1, &copyregion});

				barriers = {
					{Shit::AccessFlagBits::TRANSFER_WRITE_BIT,
					 Shit::AccessFlagBits::SHADER_READ_BIT,
					// Root::getSingleton().getTransferQueue()->GetFamilyIndex(),
					// Root::getSingleton().getGraphicsQueue()->GetFamilyIndex(),
					 ST_QUEUE_FAMILY_IGNORED,
					 ST_QUEUE_FAMILY_IGNORED,
					 _buffers[index],
					 0,
					 _buffers[index]->GetCreateInfoPtr()->size},
				};
				cmdBuffer->PipelineBarrier(
					Shit::PipelineBarrierInfo{
						Shit::PipelineStageFlagBits::TRANSFER_BIT,
						Shit::PipelineStageFlagBits::FRAGMENT_SHADER_BIT,
						{},
						0,
						0,
						(uint32_t)barriers.size(),
						barriers.data(),
					});
			});
	}
	else
	{
		void *data;
		_buffers[index]->MapMemory(0, _data.size(), &data);
		memcpy(data, _data.data(), _data.size());
		_buffers[index]->UnMapMemory();
	}
}
void Buffer::destroyImpl(int index)
{
	if (!_buffers.empty())
	{
		for (auto e : _buffers)
			Root::getSingleton().getDevice()->Destroy(e);
		_buffers.clear();
	}
	if (_stageBuffer)
	{
		Root::getSingleton().getDevice()->Destroy(_stageBuffer);
		_stageBuffer = nullptr;
	}
}
//===================================
BufferManager::BufferManager() {}
void BufferManager::removeImpl(const GPUResource *res)
{
	auto p = static_cast<const Buffer *>(res);
	_hashGroup[std::string(res->getGroupName())]
		.erase(BufferPropertyDesciption(p->getBufferUsage(), p->getMemoryProperty()));
}
void BufferManager::removeGroupImpl(std::string_view group)
{
	_hashGroup.erase(std::string(group));
}
void BufferManager::removeAllImpl()
{
	_hashGroup.clear();
}
void BufferManager::createGroupImpl(std::string_view group)
{
	_hashGroup.emplace(std::string(group), std::unordered_map<BufferPropertyDesciption, GPUResource *>{});
}
Buffer *BufferManager::createOrRetriveBuffer(
	const BufferPropertyDesciption &bufferDescription,
	std::string_view group)
{
	std::string groupName{group};
	if (!groupExists(group))
		createGroup(group);
	auto it = _hashGroup[groupName].find(bufferDescription);
	if (it != _hashGroup[groupName].cend())
		return static_cast<Buffer *>(it->second);

	size_t offsetAlignment = 16;
	if (
		bool(bufferDescription.usage & Shit::BufferUsageFlagBits::UNIFORM_BUFFER_BIT) ||
		bool(bufferDescription.usage & Shit::BufferUsageFlagBits::STORAGE_TEXEL_BUFFER_BIT) ||
		bool(bufferDescription.usage & Shit::BufferUsageFlagBits::UNIFORM_TEXEL_BUFFER_BIT))
		offsetAlignment = 0x100;
	else if (bool(bufferDescription.usage & Shit::BufferUsageFlagBits::STORAGE_BUFFER_BIT))
		offsetAlignment = 0x20;
	auto a = new Buffer(this, group, bufferDescription.usage,
						bufferDescription.memoryPropertyFlag, bufferDescription.frameDependent, offsetAlignment);
	add(a);
	_hashGroup[groupName].emplace(bufferDescription, a);
	return a;
}
Buffer *BufferManager::getBuffer(
	const BufferPropertyDesciption &bufferDescription,
	std::string_view group) const
{
	return static_cast<Buffer *>(_hashGroup.at(std::string(group)).at(bufferDescription));
}
// IdType BufferManager::addBufferData(
//	Shit::BufferUsageFlagBits usage,
//	Shit::MemoryPropertyFlagBits memoryProperty,
//	std::string_view group,
//	const void *data,
//	size_t stride,
//	size_t count)
//{
//	return addBufferData(createOrRetrive(usage, memoryProperty, group), data, stride, count);
// }
// IdType BufferManager::addBufferData(Buffer *buffer, const void *data, size_t stride, size_t count)
//{
//	auto offset = buffer->size();
//	buffer->addDataImpl(data, stride * count);
//	auto a =new BufferView{buffer, static_cast<uint32_t>(offset), stride, count};
//	return _bufferViewMap.emplace(a->getId(), std::unique_ptr<BufferView>(a)).first->second->getId();
// }
// IdType BufferManager::addBufferData(IdType bufferId, const void *data, size_t stride, size_t count)
//{
//	return static_cast<Buffer *>(_hashGroup.at(group).at(BufferPropertyDesciption{usage, memoryProperty}));
// }