#include "collider.hpp"
#include "renderable.hpp"
#include "sceneNode.hpp"

Collider::Collider(SceneNode *parentNode) : Component(parentNode)
{
	_name = "Collider" + std::to_string(getId());
}
void Collider::generateBoundingVolume()
{
	std::vector<Renderable *> renderables;
	_parent->getComponents(renderables, _includeChildNodes);
	for (auto p : renderables)
	{
		_boundingVolume.merge(p->getBoundingVolume());
	}
}
void Collider::prepare()
{
	generateBoundingVolume();
}