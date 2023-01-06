#pragma once
#include "prerequisites.hpp"
#include "boundingVolume.hpp"
#include "component.hpp"

class Collider : public Component
{
	BoundingVolume _boundingVolume{};
	bool _includeChildNodes{true};

	void generateBoundingVolume();

public:
	Collider(SceneNode *parentNode);

	constexpr void setincludeChildNodes(bool val) { _includeChildNodes = val; }

	constexpr BoundingVolume const &getBoundingVolume() const { return _boundingVolume; }

	//void onBoundingVolumeChanged();

	void prepare() override;

	NODISCARD Component *clone() override
	{
		return new Collider(*this);
	}
};