#include "sceneNode.hpp"
#include "behaviour.hpp"
#include "scene.hpp"
#include "light.hpp"
#include "meshView.hpp"
#include "animation.hpp"
#include "collider.hpp"
#include "root.hpp"
#include "buffer.hpp"

SceneNode::SceneNode(bool isStatic, Scene *scene) : _static(isStatic), _scene(scene)
{
	setName("scene node" + std::to_string(getId()));
	init();
}
SceneNode::SceneNode(std::string_view name, bool isStatic, Scene *scene)
	: Node(name), _static(isStatic), _scene(scene)
{
	init();
}
SceneNode::SceneNode(SceneNode const &other) : Node(other)
{
	//_creator = other._creator;
	_initialTransform = other._initialTransform;
	_localTransform = other._localTransform;
	_globalTransformMatrix = other._globalTransformMatrix;
	_inheritRotation = other._inheritRotation;
	_inheritScale = other._inheritScale;
	_createDescriptorSet = other._createDescriptorSet;
	_static = other._static;

	for (auto &&e : other._components)
	{
		_components.emplace_back(std::unique_ptr<Component>(e->clone()))->setParent(this);
	}
}
Node *SceneNode::clone(bool recursive)
{
	auto ret = new SceneNode(*this);
	if (recursive)
	{
		for (auto &&e : _children)
		{
			ret->addChild(e->clone(true));
		}
	}
	// animation component is special
	std::vector<Animation *> animations;
	ret->getComponents(animations, recursive);
	for (auto p : animations)
	{
		p->setParent(ret);
	}
	return ret;
}
//void SceneNode::copyChildNode(Node *childNode)
//{
//	_children.emplace_back(std::make_unique<SceneNode>(*static_cast<SceneNode *>(childNode)));
//}
//SceneNode &SceneNode::operator=(SceneNode const &other)
//{
//	Node::operator=(other);

//	//_creator = other._creator;
//	_initialTransform = other._initialTransform;
//	_localTransform = other._localTransform;
//	_globalTransformMatrix = other._globalTransformMatrix;
//	_inheritRotation = other._inheritRotation;
//	_inheritScale = other._inheritScale;
//	copyComponents(other._components);
//	return *this;
//}
void SceneNode::init()
{
	setInitialState({
		glm::vec3(0),
		glm::vec3(1),
		glm::quat(1, 0, 0, 0) // wxyz
	});
	resetToInitialState();
}
void SceneNode::prepareDescriptorSet()
{
	auto descriptorManager = Root::getSingleton().getDescriptorManager();
	auto bufferManager = Root::getSingleton().getBufferManager();

	Buffer *buffer;
	Shit::MemoryPropertyFlagBits memoryProperty;
	if (_static)
	{
		memoryProperty = Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT;
	}
	else
	{
		// dynamic node
		memoryProperty = Shit::MemoryPropertyFlagBits::HOST_VISIBLE_BIT | Shit::MemoryPropertyFlagBits::HOST_COHERENT_BIT;
	}
	buffer = bufferManager->createOrRetriveBuffer(
		BufferPropertyDesciption{
			Shit::BufferUsageFlagBits::UNIFORM_BUFFER_BIT | Shit::BufferUsageFlagBits::TRANSFER_DST_BIT,
			memoryProperty,
			!_static});

	_descriptorSetData = Root::getSingleton().getDescriptorManager()->createDescriptorSetData(
		Root::getSingleton().getDescriptorSetLayoutNode(), !_static);

	_transformBufferView = buffer->allocate(sizeof(float) * 16, 1);
	_descriptorSetData->setBuffers(DESCRIPTOR_BINDING_NODE, 0, {&_transformBufferView, 1});
}
void SceneNode::addChildImpl(Node *node)
{
	auto p = static_cast<SceneNode *>(node);
	if (p->_static != _static && _static == false)
	{
		LOG("static scenenode should not attach to a dynamic scenenode")
	}

	if (_scene)
		static_cast<SceneNode *>(node)->setScene(_scene);
}
void SceneNode::setScene(Scene *scene)
{
	_scene = scene;
	for (auto &&e : _components)
	{
		_scene->_addComponent(e.get());
	}
	for (auto &&e : _children)
	{
		static_cast<SceneNode *>(e.get())->setScene(scene);
	}
}
void SceneNode::enable(bool flag)
{
	_enable = flag;
	for (auto &&e : _children)
	{
		static_cast<SceneNode *>(e.get())->enable(flag);
	}
}
void SceneNode::prepareImpl()
{
	Node::prepareImpl();
	//create ubo descriptorSet
	if (_createDescriptorSet)
	{
		prepareDescriptorSet();
	}
	// TRS
	_globalTransformMatrix = glm::mat4_cast(_localTransform.rotation) * glm::scale(glm::mat4(1), _localTransform.scale);
	_globalTransformMatrix[3].x += _localTransform.translation.x;
	_globalTransformMatrix[3].y += _localTransform.translation.y;
	_globalTransformMatrix[3].z += _localTransform.translation.z;

	if (_parent)
	{
		// if (_inheritScale)
		//{
		//	_globalTransform.matrix = glm::scale(_localTransform.matrix, _localTransform.scale);
		// }
		// if (_inheritRotation)
		//{
		//	_globalTransform.matrix = glm::mat4_cast(_localTransform.rotation) * _globalTransform.matrix;
		// }
		_globalTransformMatrix = static_cast<SceneNode *>(_parent)->getGlobalTransformMatrix() * _globalTransformMatrix;
		//_globalTransform.rotation = static_cast<SceneNode *>(_parent)->getGlobalTransform().rotation * _globalTransform.rotation;
		//_globalTransform.translation = static_cast<SceneNode *>(_parent)->getGlobalTransform().translation * _globalTransform.translation;
		//_globalTransform.scale = static_cast<SceneNode *>(_parent)->getGlobalTransform().scale * _globalTransform.scale;
	}

	for (auto &&p : _components)
	{
		p->prepare();
	}
}
void SceneNode::updateImpl()
{
	Node::updateImpl();
	// TRS
	_globalTransformMatrix = glm::mat4_cast(_localTransform.rotation) * glm::scale(glm::mat4(1), _localTransform.scale);
	_globalTransformMatrix[3].x += _localTransform.translation.x;
	_globalTransformMatrix[3].y += _localTransform.translation.y;
	_globalTransformMatrix[3].z += _localTransform.translation.z;

	if (_parent)
	{
		// if (_inheritScale)
		//{
		//	_globalTransform.matrix = glm::scale(_localTransform.matrix, _localTransform.scale);
		// }
		// if (_inheritRotation)
		//{
		//	_globalTransform.matrix = glm::mat4_cast(_localTransform.rotation) * _globalTransform.matrix;
		// }
		_globalTransformMatrix = static_cast<SceneNode *>(_parent)->getGlobalTransformMatrix() * _globalTransformMatrix;
		//_globalTransform.rotation = static_cast<SceneNode *>(_parent)->getGlobalTransform().rotation * _globalTransform.rotation;
		//_globalTransform.translation = static_cast<SceneNode *>(_parent)->getGlobalTransform().translation * _globalTransform.translation;
		//_globalTransform.scale = static_cast<SceneNode *>(_parent)->getGlobalTransform().scale * _globalTransform.scale;
	}

	//update all components
	for (auto &&e : _components)
	{
		e->onNodeUpdated();
	}

	if (_descriptorSetData)
	{
		_transformBufferView.buffer->setData(
			_transformBufferView.offset,
			_transformBufferView.size(),
			&_globalTransformMatrix);
	}
}
SceneNode *SceneNode::translate(const glm::vec3 d, TransformSpace space)
{
	switch (space)
	{
	case TransformSpace::LOCAL:
		_localTransform.translation += _localTransform.rotation * d;
		break;
	case TransformSpace::PARENT:
		_localTransform.translation += d;
		break;
	case TransformSpace::WORLD:
	{
		glm::mat4 temp{1};
		if (_parent)
			temp = glm::inverse(static_cast<SceneNode *>(_parent)->getGlobalTransformMatrix());
		_localTransform.translation += glm::vec3(temp * glm::vec4(d, 1));
		break;
	}
	default:
		return this;
	}
	needUpdate();
	return this;
}
SceneNode *SceneNode::rotate(const glm::quat &rotation, TransformSpace space)
{
	switch (space)
	{
	case TransformSpace::LOCAL:
		_localTransform.rotation *= rotation;
		break;
	case TransformSpace::PARENT:
		_localTransform.rotation = rotation * _localTransform.rotation;
		break;
	case TransformSpace::WORLD:
		_globalTransformMatrix = getGlobalTransformMatrix();
		_localTransform.rotation = glm::quat_cast(glm::inverse(_globalTransformMatrix) * glm::mat4_cast(rotation) * _globalTransformMatrix) *
								   _localTransform.rotation;
		break;
	}
	needUpdate();
	return this;
}
SceneNode *SceneNode::scale(const glm::vec3 &scale, TransformSpace space)
{
	switch (space)
	{
	case TransformSpace::LOCAL:
		_localTransform.scale *= scale;
		break;
	case TransformSpace::PARENT:
		_localTransform.scale *= scale * _localTransform.rotation;
		break;
	case TransformSpace::WORLD:
	{
		_globalTransformMatrix = getGlobalTransformMatrix();
		_localTransform.rotation = glm::quat_cast(glm::inverse(_globalTransformMatrix) * glm::scale(glm::mat4(1), scale) * _globalTransformMatrix) *
								   _localTransform.rotation;
		break;
	}
	}
	needUpdate();
	return this;
}
void SceneNode::setInitialState(const Transform transform)
{
	_initialTransform = transform;
}
void SceneNode::resetToInitialState()
{
	_localTransform = _initialTransform;
	needUpdate();
}
void SceneNode::setLocalTransform(const Transform &transform)
{
	_localTransform = transform;
	needUpdate();
}
void SceneNode::setLocalTranslation(glm::vec3 const &translation)
{
	_localTransform.translation = translation;
	needUpdate();
}
void SceneNode::setLocalScale(glm::vec3 const &scale)
{
	_localTransform.scale = scale;
	needUpdate();
}
void SceneNode::setLocalRotation(glm::quat const &rotation)
{
	_localTransform.rotation = rotation;
	needUpdate();
}
void SceneNode::addComponent(Component *component)
{
	_components.emplace_back(std::unique_ptr<Component>(component))->setParent(this);
	if (_scene)
	{
		_scene->_addComponent(component);
	}
}
void SceneNode::removeComponent(Component *component)
{
	// removeUpdateListener(std::bind(&Component::parentNodeUpdateListener, component, std::placeholders::_1));
	std::erase_if(_components, [component](auto &&e)
				  { return e.get() == component; });
	if (_scene)
	{
		_scene->_removeComponent(component);
	}
}
void SceneNode::getComponents(std::vector<Component *> &components, bool recursive) const
{
	if (recursive)
	{
		getComponentsHelper(this, components);
	}
	else
	{
		for (auto &&e : _components)
		{
			components.emplace_back(e.get());
		}
	}
}
void SceneNode::getComponentsHelper(SceneNode const *node, std::vector<Component *> &components) const
{
	for (auto &&e : node->_components)
	{
		components.emplace_back(e.get());
	}
	for (auto &&child : node->_children)
	{
		getComponentsHelper(static_cast<SceneNode const *>(child.get()), components);
	}
}
void SceneNode::renderableEventListener(Renderable *renderable, Renderable::Event event)
{
	//swit
	switch (event)
	{
	case Renderable::Event::TRANSFORMATION:
	{
		std::vector<Collider *> colliders;
		getComponents(colliders);
		for (auto p : colliders)
		{
		}
	}
	break;
	}
}