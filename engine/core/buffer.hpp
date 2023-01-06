#pragma once
#include "gpuResource.hpp"

//struct BufferAccessor
//{
//	IdType bufferViewId;
//	Shit::Format format;
//};

/**
 * @brief create buffer by usage and memory property
 * 
 */
class Buffer : public GPUResource
{
public:
	Buffer(GPUResourceManager *creator,
		   std::string_view groupName,
		   Shit::BufferUsageFlagBits usage,
		   Shit::MemoryPropertyFlagBits memoryProperty,
		   bool frameDependent = false,
		   size_t offsetAlignment = 16);

	~Buffer() override;

	constexpr const char *data() const { return _data.data(); }
	constexpr Shit::BufferUsageFlagBits getBufferUsage() const { return _usage; }
	constexpr Shit::MemoryPropertyFlagBits getMemoryProperty() const { return _memoryProperty; }

	/**
	 * @brief base alignment is 16, offset alignment is _offsetAlignment, default 16
	 * 
	 * @param stride 
	 * @param count 
	 * @param data 
	 * @return BufferView 
	 */
	BufferView allocate(size_t stride, size_t count, void const *data);
	BufferView allocate(size_t stride, size_t count, int val = 0);

	template <typename T, size_t N>
	BufferView allocate(std::span<T, N> v)
	{
		return allocate(sizeof(T), ranges::size(v), ranges::data(v));
	}

	Shit::Buffer *&getGpuBufferByIndex(uint32_t index)
	{
		if (_buffers.empty())
		{
			//THROW("buffer should be uploaded once");
			upload();
		}
		index = (std::min)(index, _gpuResourceCount - 1);
		return _buffers.at(index);
	}

	/**
	 * @brief not update buffer
	 * 
	 * @param offset 
	 * @param size 
	 * @param data 
	 */
	void setData(size_t offset, size_t size, void const *data);
	void setData(size_t offset, size_t size, int val);

	size_t size() const { return _data.size(); }

	void clear()
	{
		destroy();
		_data.clear();
	}

private:
	Shit::BufferUsageFlagBits _usage;
	Shit::MemoryPropertyFlagBits _memoryProperty;
	mutable std::vector<Shit::Buffer *> _buffers;
	mutable Shit::Buffer *_stageBuffer{};
	std::vector<char> _data;

	size_t _offsetAlignment{16}; //use std430

	void prepareImpl(int index) override;
	void uploadImpl(int index) override;
	void destroyImpl(int index) override;

	friend class BufferManager;
};

//==============================================================
struct BufferPropertyDesciption
{
	Shit::BufferUsageFlagBits usage;
	Shit::MemoryPropertyFlagBits memoryPropertyFlag;
	bool frameDependent; //if true, create a buffer for each frame
};

template <>
struct std::hash<BufferPropertyDesciption>
{
	std::size_t operator()(BufferPropertyDesciption const &s) const noexcept
	{
		size_t h = std::hash<Shit::BufferUsageFlagBits>{}(s.usage);
		hashCombine(h, std::hash<Shit::MemoryPropertyFlagBits>{}(s.memoryPropertyFlag));
		hashCombine(h, std::hash<bool>{}(s.frameDependent));
		return h;
	}
};
constexpr bool operator==(const BufferPropertyDesciption &lhs, const BufferPropertyDesciption &rhs) noexcept
{
	return lhs.usage == rhs.usage &&
		   lhs.memoryPropertyFlag == rhs.memoryPropertyFlag &&
		   lhs.frameDependent == rhs.frameDependent;
}

class BufferManager : public GPUResourceManager
{
public:
	BufferManager();
	~BufferManager() {}

	Buffer *getBuffer(
		const BufferPropertyDesciption &bufferDescription,
		std::string_view group = DEFAULT_GROUP_NAME) const;

	Buffer *createOrRetriveBuffer(
		const BufferPropertyDesciption &bufferDescription,
		std::string_view group = DEFAULT_GROUP_NAME);

private:
	std::unordered_map<std::string, std::unordered_map<BufferPropertyDesciption, GPUResource *>> _hashGroup;
	//std::unordered_map<IdType, std::unique_ptr<BufferView>> _bufferViewMap;

	void removeImpl(const GPUResource *res) override;
	void removeGroupImpl(std::string_view group) override;
	void removeAllImpl() override;
	void createGroupImpl(std::string_view group) override;
};
