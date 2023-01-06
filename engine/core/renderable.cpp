#include "renderable.hpp"
#include "root.hpp"
#include "scene.hpp"
#include "camera.hpp"
#include "material.hpp"
#include "descriptorManager.hpp"
#include "buffer.hpp"
#include "sceneNode.hpp"

Renderable::Renderable(SceneNode *parent, RenderableType renderableType)
	: Component(parent), _renderableType(renderableType)
{
	_name = "renderable" + std::to_string(getId());
	_parent->enableDescriptorSet();
	addSignalListener(std::bind(&SceneNode::renderableEventListener, _parent, std::placeholders::_1, std::placeholders::_2));
}
Renderable::Renderable(SceneNode *parent, RenderableType renderableType, std::string_view name)
	: Component(parent),  _renderableType(renderableType)
{
	_parent->enableDescriptorSet();
	addSignalListener(std::bind(&SceneNode::renderableEventListener, _parent, std::placeholders::_1, std::placeholders::_2));
}
Renderable::Renderable(Renderable const &other) : Component(other)
{
	_renderableType = other._renderableType;
	_boundingVolume = other._boundingVolume;
	_lightMasks = other._lightMasks;
	_enable = other._enable;
	_renderPriority = other._renderPriority;
	_renderLayerMask = other._renderLayerMask;

	_receiveShadow = other._receiveShadow;
	_castShadow = other._castShadow;
	_staticShadowCaster = other._staticShadowCaster;

	//copy primitive views
	_primitiveViews.reserve(other._primitiveViews.size());
	for (auto &&e : other._primitiveViews)
	{
		_primitiveViews.emplace_back(std::make_unique<PrimitiveView>(*e))->_parent = this;
	}
	addSignalListener(std::bind(&SceneNode::renderableEventListener, _parent, std::placeholders::_1, std::placeholders::_2));
}
//Renderable &Renderable::operator=(Renderable const &other)
//{
//	Component::operator=(other);
//	_renderableType = other._renderableType;
//	_boundingVolume = other._boundingVolume;
//	_frameDependent = other._frameDependent;
//	_lightMasks = other._lightMasks;
//	_enable = other._enable;
//	_renderPriority = other._renderPriority;
//	_renderLayerMask = other._renderLayerMask;

//	_receiveShadow = other._receiveShadow;
//	_castShadow = other._castShadow;
//	_staticShadowCaster = other._staticShadowCaster;

//	//copy primitive views
//	_primitiveViews.reserve(other._primitiveViews.size());
//	for (auto &&e : other._primitiveViews)
//	{
//		_primitiveViews.emplace_back(std::make_unique<PrimitiveView>(*e))->_parent = this;
//	}
//	return *this;
//}
void Renderable::prepare()
{
	_boundingVolume.transform(_parent->getGlobalTransformMatrix());
	for (auto &&p : _primitiveViews)
	{
		p->prepare();
	}
	_signal(this, Event::TRANSFORMATION);
}
void Renderable::onNodeUpdated()
{
	_boundingVolume.transform(_parent->getGlobalTransformMatrix());
	_signal(this, Event::TRANSFORMATION);
}
void Renderable::updateGPUData(uint32_t frameIndex)
{
	_parent->getDescriptorSetData()->upload(frameIndex);
	//for (auto p : _primitiveViews)
	//{
	//	p->updateGPUData(frameIndex);
	//}
}
//void Renderable::postRender(uint32_t frameIndex)
//{
//}
void Renderable::recordCommandBuffer(Shit::CommandBuffer *cmdBuffer, uint32_t frameIndex)
{
	_parent->getDescriptorSetData()->bind(
		cmdBuffer,
		Shit::PipelineBindPoint::GRAPHICS,
		Root::getSingleton().getCommonPipelineLayout(),
		DESCRIPTORSET_NODE, frameIndex);
}
void Renderable::getAllPrimitiveViews(std::vector<PrimitiveView *> &primitiveViews)
{
	for (auto &&e : _primitiveViews)
	{
		primitiveViews.emplace_back(e.get());
	}
}