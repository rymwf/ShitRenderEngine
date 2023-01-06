#include "scene.hpp"
#include "camera.hpp"
#include "root.hpp"
#include "texture.hpp"
#include "modelResource.hpp"
#include "sceneNode.hpp"
#include "renderQueue.hpp"
#include "light.hpp"

//=======================================================
Scene::Scene() 
{
	static int i = 0;
	_name = "_scene" + std::to_string(i++);
	init();
}
Scene::Scene(std::string_view name) : _name(name)
{
	init();
}
void Scene::init()
{
	// create dummy root node
	_rootNode = std::make_unique<SceneNode>("RootNode", true, this);

	auto commandPool = Root::getSingleton().getCommandPool();
	auto swapchain = Root::getSingleton().getScreen()->getSwapchain();
	auto imageCount = swapchain->GetImageCount();

	// create editor camera
	_editorCameraNode = std::make_unique<SceneNode>(true, this);
	_editorCamera = _editorCameraNode->addComponent<Camera>();
	_editorCameraNode->addComponent<EditCameraController>();
	_activeCamera = _editorCamera;

	_activeCamera = createCamera("MainCamera");
	_activeCamera->getParentNode()->translate(glm::vec3(0, 2, 10));

	_renderCamera = _editorCamera;

	//add env
	_env = std::make_unique<Environment>(this);
}
void Scene::prepare()
{
	getRootNode()->prepare(true);
	_editorCameraNode->prepare(true);

	getRootNode()->update(true);
	_editorCameraNode->update(true);
}
void Scene::preRender(uint32_t frameIndex)
{
	// 1. update monobehaviours
	updateBehaviours();

	//2 update scenenodes, and renderable components;
	getRootNode()->update(true);
	_editorCameraNode->update(true);
}
void Scene::render(uint32_t frameIndex, uint32_t viewLayer)
{
	preRender(frameIndex);
	_renderCamera->render(frameIndex);
	postRender(frameIndex);
}
void Scene::postRender(uint32_t frameIndex)
{
	//_renderQueue.postRender(frameIndex);
	//getRootNode()->postRender(frameIndex);
	//_editorCamera->postRender(frameIndex);
}
void Scene::setSelectedNode(SceneNode *sceneNode)
{
	_selectedNodes.clear();
	if (sceneNode)
	{
		_selectedNodes.emplace_back(sceneNode);
	}
}
void Scene::addSelectedNode(SceneNode *sceneNode)
{
	_selectedNodes.emplace_back();
}
bool Scene::containSelectedNode(SceneNode *sceneNode)
{
	return ranges::find(_selectedNodes, sceneNode) != ranges::end(_selectedNodes);
}
void Scene::updateBehaviours()
{
	if (_behaviourGroup.needResort)
	{
		_behaviourGroup.sort();
		_behaviourGroup.needResort = false;
	}
	for (auto p : _behaviourGroup.behaviours)
	{
		p->update();
	}
}
SceneNode *Scene::createSceneNode(std::string_view name, bool isStatic, SceneNode *parent)
{
	if (!parent)
		parent = getRootNode();
	return static_cast<SceneNode *>(parent->createChild(name, isStatic));
}
SceneNode *Scene::addSceneNode(SceneNode *sceneNode, SceneNode *parent)
{
	sceneNode->_scene = this;
	if (!parent)
		parent = getRootNode();
	parent->addChild(sceneNode);

	// add behaviours
	sceneNode->getComponents(_behaviourGroup.behaviours, true);
	_behaviourGroup.needResort = true;
	return sceneNode;
}
Model *Scene::createModel(ModelResource *modelResource, SceneNode *parent)
{
	if (!parent)
		parent = getRootNode();
	auto p = modelResource->getModel()->clone();
	parent->addChild(p);
	return static_cast<Model *>(p);
}
Camera *Scene::createCamera(std::string_view name, SceneNode *parentNode)
{
	auto p = createSceneNode(name, false, parentNode);
	auto ret = p->addComponent<Camera>();
	return ret;
}
Light *Scene::createLight(std::string_view name, SceneNode *parentNode)
{
	auto p = createSceneNode(name, false, parentNode);
	return p->addComponent<Light>();
}
void Scene::_addComponent(Component *component)
{
	if (auto p = dynamic_cast<Renderable *>(component))
	{
		std::vector<PrimitiveView *> primitiveViews;
		p->getAllPrimitiveViews(primitiveViews);
	}
	else if (auto p = dynamic_cast<Behaviour *>(component))
	{
		_behaviourGroup.behaviours.emplace_back(p);
		_behaviourGroup.needResort = true;
	}
	else if (auto p = dynamic_cast<Light *>(component))
	{
		_lights.emplace_back(p);
		_editorCamera->addUpdateListener(std::bind(&Light::cameraListener, p, std::placeholders::_1));
	}
}
void Scene::_addComponents(std::span<Component *> components)
{
	for (auto p : components)
		_addComponent(p);
}
void Scene::_removeComponent(Component *component)
{
	if (auto p = dynamic_cast<Renderable *>(component))
	{
		std::vector<PrimitiveView *> primitiveViews;
		p->getAllPrimitiveViews(primitiveViews);
	}
	else if (auto p = dynamic_cast<Behaviour *>(component))
	{
		std::erase(_behaviourGroup.behaviours, component);
	}
	else if (auto p = dynamic_cast<Light *>(component))
	{
		std::erase(_lights, component);
	}
}
void Scene::_removeComponents(std::span<Component *> components)
{
	for (auto p : components)
		_removeComponent(p);
}