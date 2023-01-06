#pragma once
#include "prerequisites.hpp"
#include "behaviour.hpp"
#include "renderable.hpp"
#include "camera.hpp"
#include "renderQueue.hpp"
#include "sceneNode.hpp"
#include "environment.hpp"

enum class ViewLayerType
{
	FORWARD,
	DEFERRED,
	// DEFERRED_LEGACY,
	// VERTEX_LIT,
};

class Scene
{
public:
	enum class Event
	{
	};

	Scene();
	Scene(std::string_view name);

	virtual ~Scene() {}

	void windowEventCallback(Shit::Event const &event);

	constexpr std::string_view getName() const { return _name; }

	Camera *getRenderCamera() const { return _renderCamera; }

	void renderActiveCamera() { _renderCamera = _activeCamera; }
	void renderEditorCamera() { _renderCamera = _editorCamera; }

	Camera *getEditorCamera() const { return _editorCamera; }
	Camera *getActiveCamera() const { return _activeCamera; }

	void activeCamera(Camera *camera) { _activeCamera = camera; }

	// submit viewlayer primary command buffers
	void render(uint32_t frameIndex, uint32_t viewLayer);

	void prepare();

	/**
	 * @brief clear all previous selected nodes
	 *
	 * @param sceneNode can be nullptr
	 */
	void setSelectedNode(SceneNode *sceneNode);
	void addSelectedNode(SceneNode *sceneNode);
	constexpr std::span<SceneNode *> getSeletectedNodes() { return _selectedNodes; }
	bool containSelectedNode(SceneNode *sceneNode);

	void updateBehaviours();

	SceneNode *getRootNode() const { return _rootNode.get(); }

	SceneNode *createSceneNode(
		std::string_view name = {},
		bool isStatic = true,
		SceneNode *parent = nullptr);
	Model *createModel(ModelResource *modelResource, SceneNode *parent = nullptr);

	SceneNode *addSceneNode(SceneNode *sceneNode, SceneNode *parent = nullptr);

	Camera *createCamera(std::string_view name = {}, SceneNode *parentNode = nullptr);

	Light *createLight(std:: string_view name = {}, SceneNode *parentNode = nullptr);

	constexpr std::span<Light *> getLights() { return _lights; }

	Environment *getEnvironment() const { return _env.get(); }

protected:
	std::string _name;

	std::unique_ptr<SceneNode> _rootNode;

	std::vector<Light *> _lights;

	std::unique_ptr<SceneNode> _editorCameraNode;
	Camera *_editorCamera{};
	Camera *_activeCamera{};
	Camera *_renderCamera{};

	//std::array<std::vector<Renderable *>, (size_t)MaterialType::Num> _renderables;
	std::unique_ptr<Environment> _env;

	std::vector<SceneNode *> _selectedNodes;

	struct BehaviourGroup
	{
		struct Comp
		{
			bool operator()(Behaviour *l, Behaviour *r) const noexcept
			{
				return l->getPriority() < r->getPriority();
			}
		};

		std::vector<Behaviour *> behaviours;
		bool needResort = true;
		//bool needClear = false;
		void sort()
		{
			ranges::sort(behaviours, Comp{});
		}
	};
	BehaviourGroup _behaviourGroup;

	void init();

	// update all subscenes
	void preRender(uint32_t frameIndex);
	void postRender(uint32_t frameIndex);

	/**
  * @brief called by SceneNode
  * 
  * @param component 
  */
	void _addComponent(Component *component);
	void _addComponents(std::span<Component *> components);

	void _removeComponent(Component *component);
	void _removeComponents(std::span<Component *> components);

	friend class SceneNode;
};
