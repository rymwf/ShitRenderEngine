#include "boundingVolume.hpp"

glm::vec3 AxisAlignedBox::getCenter() const
{
	return (minValue + maxValue) / glm::vec3(2);
}
glm::vec3 AxisAlignedBox::getSize() const
{
	return maxValue - minValue;
}
AxisAlignedBox::Corners AxisAlignedBox::getAllCorners() const
{
	return AxisAlignedBox::Corners{
		getCorner(static_cast<AxisAlignedBox::CornerEnum>(0)),
		getCorner(static_cast<AxisAlignedBox::CornerEnum>(1)),
		getCorner(static_cast<AxisAlignedBox::CornerEnum>(2)),
		getCorner(static_cast<AxisAlignedBox::CornerEnum>(3)),
		getCorner(static_cast<AxisAlignedBox::CornerEnum>(4)),
		getCorner(static_cast<AxisAlignedBox::CornerEnum>(5)),
		getCorner(static_cast<AxisAlignedBox::CornerEnum>(6)),
		getCorner(static_cast<AxisAlignedBox::CornerEnum>(7))};
}
glm::vec3 AxisAlignedBox::getCorner(AxisAlignedBox::CornerEnum corner) const
{
	switch (corner)
	{
	case AxisAlignedBox::CornerEnum::FAR_LEFT_BOTTOM:
		return minValue;
	case AxisAlignedBox::CornerEnum::FAR_LEFT_TOP:
		return glm::vec3(minValue.x, maxValue.y, minValue.z);
	case AxisAlignedBox::CornerEnum::FAR_RIGHT_TOP:
		return glm::vec3(maxValue.x, maxValue.y, minValue.z);
	case AxisAlignedBox::CornerEnum::FAR_RIGHT_BOTTOM:
		return glm::vec3(maxValue.x, minValue.y, minValue.z);
	case AxisAlignedBox::CornerEnum::NEAR_RIGHT_BOTTOM:
		return glm::vec3(maxValue.x, minValue.y, maxValue.z);
	case AxisAlignedBox::CornerEnum::NEAR_LEFT_BOTTOM:
		return glm::vec3(minValue.x, minValue.y, maxValue.z);
	case AxisAlignedBox::CornerEnum::NEAR_LEFT_TOP:
		return glm::vec3(minValue.x, maxValue.y, maxValue.z);
	case AxisAlignedBox::CornerEnum::NEAR_RIGHT_TOP:
		return maxValue;
	default:
		return glm::vec3();
	}
}

void AxisAlignedBox::merge(const glm::vec3 &point)
{
	switch (extent)
	{
	case BoundingVolumeExtentType::NONE:
		minValue = maxValue = point;
		extent = BoundingVolumeExtentType::FINITE;
		break;
	case BoundingVolumeExtentType::FINITE:
		minValue = glm::min(minValue, point);
		maxValue = glm::max(maxValue, point);
		break;
	case BoundingVolumeExtentType::INFINITE:
		break;
	}
}
void AxisAlignedBox::merge(const AxisAlignedBox &aabbBox)
{
	if (extent == BoundingVolumeExtentType::INFINITE || aabbBox.extent == BoundingVolumeExtentType::NONE)
		return;
	else if (extent == BoundingVolumeExtentType::NONE)
		*this = aabbBox;
	else if (aabbBox.extent == BoundingVolumeExtentType::INFINITE)
		extent = BoundingVolumeExtentType::INFINITE;
	else
	{
		minValue = glm::min(minValue, aabbBox.minValue);
		maxValue = glm::max(minValue, aabbBox.maxValue);
	}
}
void AxisAlignedBox::transform(const glm::mat3x4 &m)
{
	if (extent != BoundingVolumeExtentType::FINITE)
		return;
	glm::vec3 oldMin = minValue, oldMax = maxValue;
	glm::vec4 currentCorner = glm::vec4(oldMin, 1);
	extent = BoundingVolumeExtentType::NONE;

	merge(m * currentCorner);

	// min,min,max
	currentCorner.z = oldMax.z;
	merge(m * currentCorner);

	// min max max
	currentCorner.y = oldMax.y;
	merge(m * currentCorner);

	// min max min
	currentCorner.z = oldMin.z;
	merge(m * currentCorner);

	// max max min
	currentCorner.x = oldMax.x;
	merge(m * currentCorner);

	// max max max
	currentCorner.z = oldMax.z;
	merge(m * currentCorner);

	// max min max
	currentCorner.y = oldMin.y;
	merge(m * currentCorner);

	// max min min
	currentCorner.z = oldMin.z;
	merge(m * currentCorner);
}
void AxisAlignedBox::scale(const glm::vec3 &s)
{
	if (extent != BoundingVolumeExtentType::FINITE)
		return;
	minValue *= s;
	maxValue *= s;
}
void BoundingVolume::merge(BoundingVolume const &other)
{
	box.merge(other.box);
}
void BoundingVolume::merge(const glm::vec3 &point)
{
	box.merge(point);
}
void BoundingVolume::transform(const glm::mat3x4 &m)
{
	box.transform(m);
}