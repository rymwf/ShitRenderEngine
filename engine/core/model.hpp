#pragma once
#include "sceneNode.hpp"
#include "animation.hpp"

class Model : public SceneNode
{
	ModelResource *_modelResource;
	std::vector<SceneNode *> _allChildren{};
	std::vector<std::unique_ptr<Skin>> _skins;

	Node *cloneHelper(Model *srcRoot, Model *dstRoot, Node *srcNode);

	int getChildIndex(SceneNode *node) const;

public:
	Model(ModelResource *modelResource, Scene *scene = nullptr);
	Model(std::string_view name, ModelResource *modelResource, Scene *scene = nullptr);
	Model(Model const &other);

	~Model() {}

	constexpr ModelResource *getModelResource() const { return _modelResource; }

	BoundingVolume getBoundingVolume() const;

	NODISCARD Node *clone(bool recursive = true) override;

	Skin *createSkin(size_t count, int *jointNodeIndices);
	Skin *getSkin(size_t index = 0) const { return _skins.at(index).get(); }
	Skin *getSkin(std::string_view name) const
	{
		for (auto &&e : _skins)
		{
			if (e->getName() == name)
				return e.get();
		}
		return nullptr;
	}

	void setChildIndex(SceneNode *node, size_t index);

	constexpr SceneNode *getChildByIndex(size_t index) const
	{
		if (index >= _allChildren.size())
			return nullptr;
		return _allChildren.at(index);
	}
};