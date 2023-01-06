#pragma once
#include "prerequisites.hpp"
#include "idObject.hpp"
#include <glm/glm.hpp>

class Component : public IdObject<Component>
{
	Component &operator=(Component const &other);

protected:
	SceneNode *_parent;
	std::string _name;

	bool _enable = true;

	virtual void setParentImpl(SceneNode *parent) {}

public:
	Component(SceneNode *parent = nullptr);
	Component(SceneNode *parent, std::string_view name) : _parent(parent), _name(name) {}

	Component(Component const &other);

	virtual ~Component() {}

	constexpr std::string_view getName() const { return _name; }
	void setName(std::string_view name) { _name = name; }

	void setParent(SceneNode *parent)
	{
		_parent = parent;
		setParentImpl(parent);
	}

	bool isEnable() const;
	void enable(bool val) { _enable = val; }

	constexpr SceneNode *getParentNode() const { return _parent; }

	/**
	 * @brief called by scenendoe
	 * 
	 */
	virtual void prepare() {}

	virtual void onNodeUpdated() {}

	virtual void updateGPUData(uint32_t frameIndex) {}
	//virtual void postRender(uint32_t frameIndex) {}

	NODISCARD virtual Component *clone() = 0;
};
