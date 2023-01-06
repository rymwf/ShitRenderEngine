#pragma once
#include <glm/glm.hpp>
#include <array>

enum class BoundingVolumeExtentType
{
	NONE, // empty box
	FINITE,
	INFINITE //
};

struct AxisAlignedBox
{
	glm::vec3 minValue{std::numeric_limits<float>::max()};
	glm::vec3 maxValue{std::numeric_limits<float>::lowest()};

	/*
		   1-------2
		  /|      /|
		 / |     / |
		5-------4  |
		|  0----|--3
		| /     | /
		|/      |/
		6-------7
		*/
	enum class CornerEnum
	{
		FAR_LEFT_BOTTOM,
		FAR_LEFT_TOP,
		FAR_RIGHT_TOP,
		FAR_RIGHT_BOTTOM,
		NEAR_RIGHT_BOTTOM,
		NEAR_LEFT_BOTTOM,
		NEAR_LEFT_TOP,
		NEAR_RIGHT_TOP
	};
	typedef std::array<glm::vec3, 8> Corners;

	BoundingVolumeExtentType extent{BoundingVolumeExtentType::NONE};

	glm::vec3 getCenter() const;
	glm::vec3 getSize() const;
	Corners getAllCorners() const;
	glm::vec3 getCorner(CornerEnum corner) const;

	void merge(const glm::vec3 &point);
	void merge(const AxisAlignedBox &aabbBox);
	void transform(const glm::mat3x4 &m);

	void scale(const glm::vec3 &s);
};
struct BoundingVolume
{
	//struct Box
	//{
	//	using AABB = AxisAlignedBox;
	//	AABB aabb;
	//	struct OBB
	//	{
	//	} obb;
	//}
	AxisAlignedBox box{};

	struct Sphere
	{
	} sphere{};
	struct Ellipsoid
	{
	} ellipsoid{};
	struct Cylinder
	{
	} cylinder{};

	void merge(BoundingVolume const &other);
	void merge(const glm::vec3 &point);
	void transform(const glm::mat3x4 &m);
};