#include "model.hpp"
#include "meshView.hpp"
#include "modelResource.hpp"

Model::Model(ModelResource *modelResource, Scene *scene)
	: SceneNode(scene), _modelResource(modelResource)
{
}
Model::Model(std::string_view name, ModelResource *modelResource, Scene *scene)
	: SceneNode(name, scene), _modelResource(modelResource)
{
}
Model::Model(Model const &other) : SceneNode(other)
{
	_modelResource = other._modelResource;
	// auto it = childBegin();
	// auto end = childEnd();
	// for (; it != end; ++it)
	//{
	//	// ret->addChild((*it)->clone(recursive));
	//	cloneHelper(this, const_cast<Model *>(&other), it->get());
	// }

	for (size_t i = 0; i < other._skins.size(); ++i)
	{
		_skins.emplace_back(std::make_unique<Skin>(*other._skins[i]))->setOwner(this, i);
	}
}
BoundingVolume Model::getBoundingVolume() const
{
	return _modelResource->getBoundingVolume();
}
Node *Model::clone(bool recursive)
{
	auto ret = new Model(*this);
	// clone all children
	if (recursive)
	{
		auto it = childBegin();
		auto end = childEnd();
		for (; it != end; ++it)
		{
			// ret->addChild((*it)->clone(recursive));
			ret->addChild(cloneHelper(this, ret, it->get()));
		}
	}
	// animation component is special
	std::vector<Animation *> animations;
	ret->getComponents(animations, recursive);
	for (auto p : animations)
	{
		p->setParent(ret);
	}

	for (size_t i = 0; i < _skins.size(); ++i)
	{
		_skins[i]->setOwner(ret, i);
	}
	return ret;
}
Node *Model::cloneHelper(Model *srcRoot, Model *dstRoot, Node *srcNode)
{
	auto ret = srcNode->clone(false);
	auto index = srcRoot->getChildIndex(static_cast<SceneNode *>(srcNode));
	if (index >= 0)
	{
		dstRoot->setChildIndex(static_cast<SceneNode *>(ret), index);
	}

	//copy skin
	std::vector<MeshView *> meshViews;
	static_cast<SceneNode *>(srcNode)->getComponents(meshViews);
	for (auto p : meshViews)
	{
		auto skin = p->getSkin();
		if (skin)
		{
			p->setSkin(dstRoot->getSkin(skin->getName()));
		}
	}

	// if()

	// else
	//{
	//	LOG("cannot find node ", srcNode->getId(), " in children list")
	// }

	auto it = srcNode->childBegin();
	auto end = srcNode->childEnd();
	for (; it != end; ++it)
	{
		ret->addChild(cloneHelper(srcRoot, dstRoot, it->get()));
	}
	return ret;
}
void Model::setChildIndex(SceneNode *node, size_t index)
{
	if (_allChildren.size() <= index)
		_allChildren.resize(index + 1);
	_allChildren[index] = node;
}
int Model::getChildIndex(SceneNode *node) const
{
	auto it = ranges::find(_allChildren, node);
	if (it == _allChildren.cend())
		return -1;
	return it - _allChildren.cbegin();
}
Skin *Model::createSkin(size_t count, int *jointNodeIndices)
{
	return _skins.emplace_back(std::make_unique<Skin>(count, jointNodeIndices)).get();
}