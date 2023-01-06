#include "meshView.hpp"
#include "root.hpp"
#include "buffer.hpp"
#include "sceneNode.hpp"
#include "renderable.hpp"
#include "descriptorManager.hpp"
#include "material.hpp"
#include "animation.hpp"

MeshView::MeshView(SceneNode *parent, Mesh *mesh)
	: Renderable(parent, RenderableType::MESHVIEW), _mesh(mesh)
{
	setName(_mesh->getName());
	initialise();
}
MeshView::MeshView(SceneNode *parent, Mesh *mesh, std::string_view name)
	: Renderable(parent, RenderableType::MESHVIEW, name), _mesh(mesh)
{
	initialise();
}
MeshView::MeshView(MeshView const &other) : Renderable(other)
{
	_mesh = other._mesh;
	_skin = other._skin;
}
void MeshView::initialise()
{
	_boundingVolume = _mesh->_boundingVolume;

	for (auto &&e : _mesh->_primitives)
	{
		if (e->targets.empty())
			_primitiveViews.emplace_back(std::make_unique<PrimitiveView>(this, e.get()));
		else
			_primitiveViews.emplace_back(std::make_unique<MorphPrimitiveView>(this, e.get()));
	}
}
void MeshView::setMorphWeights(std::span<float> weights)
{
	for (auto &&e : _primitiveViews)
	{
		static_cast<MorphPrimitiveView *>(e.get())->setMorphWeights(weights);
	}
}
void MeshView::prepare()
{
	Renderable::prepare();
	if (!_skin)
		_skin = Skin::sGetEmptySkin();
	_skin->prepare();
}
void MeshView::recordCommandBuffer(Shit::CommandBuffer *cmdBuffer, uint32_t frameIndex)
{
	Renderable::recordCommandBuffer(cmdBuffer, frameIndex);
	_skin->recoordCommandBuffer(cmdBuffer, frameIndex);
}