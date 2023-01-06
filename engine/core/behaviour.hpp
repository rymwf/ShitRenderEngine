/**
 * @file behaviour.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2022
 *
 */
#pragma once
#include "prerequisites.hpp"
#include "component.hpp"

class Behaviour : public Component
{
protected:
	int _priority{0};

public:
	Behaviour(SceneNode *parent);
	constexpr int getPriority() const { return _priority; }
	constexpr void setPriority(int priority) { _priority = priority; }

	virtual ~Behaviour() {}

	void prepare() override {}

	/**
	 * @brief update perloop
	 * 
	 */
	virtual void update() = 0;
	//virtual void fixedUpdate() {}
	virtual void onWindow(Shit::Event const &event) {}
	//virtual void onDisable() {}
	//virtual void onEnable() {}
};

class EditCameraController : public Behaviour
{
	// per second
	float _speed = 3.f;

public:
	EditCameraController(SceneNode *sceneNode);
	~EditCameraController() {}

	EditCameraController(EditCameraController const &other);

	NODISCARD Component *clone() override
	{
		return new EditCameraController(*this);
	}

	constexpr void setSpeed(float speed) { _speed = speed; }

	void prepare() override;
	void update() override;
	void onWindow(Shit::Event const &event) override;

private:
};