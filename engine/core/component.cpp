#include "component.hpp"
#include "sceneNode.hpp"

Component::Component(SceneNode *parent) : _parent(parent)
{
	_name = "Component" + std::to_string((int)getId());
}
Component::Component(Component const &other) : IdObject<Component>(other)
{
	_parent = other._parent;
	_name = other._name;
	_enable = other._enable;
}
bool Component::isEnable() const
{
	return _enable && _parent->isEnable();
}
// Component &Component::operator=(Component const &other)
//{
//	IdObject<Component>::operator=(other);
//	_parent = other._parent;
//	_type = other._type;
//	_name = other._name;
//	_enable = other._enable;
//	return *this;
// }