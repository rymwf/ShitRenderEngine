#include "modelResource.hpp"
#include "mesh.hpp"
#include "image.hpp"
#include "texture.hpp"
#include "collider.hpp"

ModelResource::ModelResource(std::string_view name,
							 std::string_view groupName,
							 bool manuallyLoad,
							 ManualResourceLoader *loader,
							 size_t size,
							 void const *data)
	: Resource(name, groupName, manuallyLoad, loader)
{
	if (size > 0)
	{
		_data.resize(size);
		memcpy(_data.data(), data, size);
	}
	_model = std::make_unique<Model>(name, this);
	_model->addComponent<Collider>();
}
void ModelResource::addMesh(Mesh *mesh)
{
	_meshes.emplace_back(mesh);
}
void ModelResource::addMateralDataBlock(MaterialDataBlock *materialDatablock)
{
	_materialDataBlocks.emplace_back(materialDatablock);
}
void ModelResource::addImage(Image *image)
{
	_images.emplace_back(image);
}
void ModelResource::addTexture(Texture *texture)
{
	_textures.emplace_back(texture);
}
void ModelResource::loadAllImages()
{
	for (auto p : _images)
	{
		p->load();
	}
}
AnimationClip *ModelResource::createAnimationClip(std::string_view name)
{
	if (name.empty())
		return _animationClips.emplace_back(std::make_unique<AnimationClip>()).get();
	return _animationClips.emplace_back(std::make_unique<AnimationClip>(name)).get();
}
BoundingVolume const &ModelResource::getBoundingVolume(SceneNode const *node) const
{
	BoundingVolume ret{};
	for (auto it = node->childCbegin(); it < node->childCend(); ++it)
	{
		ret.merge(getBoundingVolume(static_cast<SceneNode *>(it->get())));
	}
	std::vector<Renderable *> renderables;
	node->getComponents(renderables);
	auto &&m = node->getGlobalTransformMatrix();
	for (auto p : renderables)
	{
		auto a = p->getBoundingVolume();
		a.transform(m);
		ret.merge(a);
	}
	return ret;
}
void ModelResource::initBoundingVolume()
{
	_boundingVolume.merge(getBoundingVolume(_model.get()));
}
void ModelResource::postLoadImpl() 
{
	_model->update(true, false);
	//init bounding volume
	initBoundingVolume();
}
//BoundingVolume ModelResource::getBoundingVolume()
//{

//}