#include "sceneManager.hpp"
#include "root.hpp"
#include "scene.hpp"

SceneManager::SceneManager()
{
}
Scene *SceneManager::createScene()
{
	auto ret = new Scene();
	_scenes.emplace(ret->getName(), std::unique_ptr<Scene>(ret));
	return ret;
}
Scene *SceneManager::createScene(std::string_view name)
{
	return _scenes.emplace(name, std::make_unique<Scene>(name)).first->second.get();
}
bool SceneManager::containsScene(std::string_view name) const
{
	return _scenes.contains(std::string(name));
}
Scene *SceneManager::getScene(std::string_view name) const
{
	return _scenes.at(std::string(name)).get();
}