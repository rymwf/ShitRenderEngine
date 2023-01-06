#pragma once
#include <cstdint>
#include <string>

using IdType = uint32_t;

#define INVALID_ID ~(0UL)

template <typename T = void>
class IdObject
{
protected:
	IdType _id;

	static IdType generateId()
	{
		static IdType tempId = 0;
		return ++tempId;
	}

public:
	IdObject() : _id(generateId())
	{
	}
	virtual ~IdObject() {}
	//copy
	IdObject(const IdObject &other) : _id(generateId())
	{
	}
	IdObject &operator=(const IdObject &other)
	{
		return *this;
	}
	constexpr IdType getId() const { return _id; }
};