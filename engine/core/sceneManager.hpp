#pragma once
#include "sceneNode.hpp"
#include "buffer.hpp"
#include "meshView.hpp"
#include "animation.hpp"
#include "scene.hpp"
#include "renderable.hpp"
#include "renderQueue.hpp"
#include "pipeline.hpp"

class SceneManager
{
public:
	enum class Event
	{
	};

	SceneManager();

	Scene *createScene();
	Scene *createScene(std::string_view name);
	bool containsScene(std::string_view name) const;

	Scene *getScene(std::string_view name) const;

protected:
	void initRenderQueues();

	Shit::Signal<void(const SceneManager *, Event)> _signal;

	std::unordered_map<std::string, std::unique_ptr<Scene>> _scenes;

	friend class SceneNode;
};