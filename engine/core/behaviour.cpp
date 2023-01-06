#include "behaviour.hpp"
#include "sceneNode.hpp"
#include "camera.hpp"
#include "root.hpp"
#include "imgui-impl.hpp"

Behaviour::Behaviour(SceneNode *parent) : Component(parent)
{
	_name = "Behaviour" + std::to_string(getId());
	Root::getSingleton().getScreen()->addWindowEventCallback(
		std::bind(&Behaviour::onWindow, this, std::placeholders::_1));
}

enum DirectionBits
{
	LEFT = 1,
	RIGHT = 1 << 1,
	UP = 1 << 2,
	DOWN = 1 << 3,
	LEFT_UP = LEFT | UP,
	LEFT_DOWN = LEFT | DOWN,
	RIGHT_UP = RIGHT | UP,
	RIGHT_DOWN = RIGHT | DOWN,
};
glm::vec3 getDirectionVec(DirectionBits dir)
{
	switch (dir)
	{
	case DirectionBits::LEFT:
		return glm::vec3(-1, 0, 0);
	case DirectionBits::RIGHT:
		return glm::vec3(1, 0, 0);
	case DirectionBits::UP:
		return glm::vec3(0, 0, -1);
	case DirectionBits::DOWN:
		return glm::vec3(0, 0, 1);
	case DirectionBits::LEFT_UP:
		return glm::vec3(-1, 0, -1);
	case DirectionBits::LEFT_DOWN:
		return glm::vec3(-1, 0, 1);
	case DirectionBits::RIGHT_UP:
		return glm::vec3(1, 0, -1);
	case DirectionBits::RIGHT_DOWN:
		return glm::vec3(1, 0, 1);
	}
	return glm::vec3(0, 0, 0);
}

//====================================
EditCameraController::EditCameraController(SceneNode *sceneNode) : Behaviour(sceneNode)
{
}
EditCameraController::EditCameraController(EditCameraController const &other) : Behaviour(other)
{
}
void EditCameraController::prepare()
{
	_parent->translate(glm::vec3(0, 2, 10))
		->pitch(glm::radians(-10.f));
}
void EditCameraController::update()
{
	ImGuiIO &io = ImGui::GetIO();

	auto keyStateW = Shit::GetKeyState(Shit::KeyCode::KEY_W);
	auto keyStateA = Shit::GetKeyState(Shit::KeyCode::KEY_A);
	auto keyStateS = Shit::GetKeyState(Shit::KeyCode::KEY_S);
	auto keyStateD = Shit::GetKeyState(Shit::KeyCode::KEY_D);

	auto keyStateLeft = Shit::GetKeyState(Shit::KeyCode::KEY_LEFT);
	auto keyStateRight = Shit::GetKeyState(Shit::KeyCode::KEY_RIGHT);
	auto keyStateUp = Shit::GetKeyState(Shit::KeyCode::KEY_UP);
	auto keyStateDown = Shit::GetKeyState(Shit::KeyCode::KEY_DOWN);

	// bit0: left
	// bit1: right
	// bit2: up
	// bit3: down
	static int keystateBits;

	if (keyStateA.down || keyStateLeft.down)
		keystateBits |= 1;
	else
		keystateBits &= ~1;
	if (keyStateD.down || keyStateRight.down)
		keystateBits |= (1 << 1);
	else
		keystateBits &= ~(1 << 1);
	if (keyStateW.down || keyStateUp.down)
		keystateBits |= (1 << 2);
	else
		keystateBits &= ~(1 << 2);
	if (keyStateS.down || keyStateDown.down)
		keystateBits |= (1 << 3);
	else
		keystateBits &= ~(1 << 3);

	glm::vec3 dir = getDirectionVec((DirectionBits)keystateBits);
	if (dir.x != 0 || dir.y != 0 || dir.z != 0)
	{
		dir *= _speed * Root::getSingleton().getFrameDeltaTimeMs() / 1000;
		_parent->translate(dir, SceneNode::TransformSpace::LOCAL);
	}

	//=============================================================
	// auto keyStateMouseL = Shit::GetKeyState(Shit::KeyCode::KEY_LBUTTON);
	auto keyStateMouseR = Shit::GetKeyState(Shit::KeyCode::KEY_RBUTTON);
	auto keyStateMouseL = Shit::GetKeyState(Shit::KeyCode::KEY_LBUTTON);
	// wheel
	auto keyStateMouseMiddle = Shit::GetKeyState(Shit::KeyCode::KEY_MBUTTON);
	auto keyStateAltL = Shit::GetKeyState(Shit::KeyCode::KEY_LMENU);
	static bool flag = true;
	static int preCursorX, preCursorY, cursorX, cursorY;
	auto pWindow = Root::getSingleton().getScreen()->getWindow();

	if (keyStateMouseR.down || (keyStateMouseL.down && keyStateAltL.down))
	{
		static float c = 0.001;
		pWindow->GetCursorPos(cursorX, cursorY);
		if (flag)
		{
			pWindow->GetCursorPos(preCursorX, preCursorY);
			flag = false;
		}
		if (cursorX != preCursorX)
		{
			_parent->yaw((preCursorX - cursorX) * c, SceneNode::TransformSpace::PARENT);
		}
		if (cursorY != preCursorY)
		{
			_parent->pitch((preCursorY - cursorY) * c, SceneNode::TransformSpace::LOCAL);
		}
		preCursorX = cursorX, preCursorY = cursorY;
	}
	else if (keyStateMouseMiddle.down)
	{
		static float c = 0.001;
		c = 0.001 * glm::length(_parent->getGlobalPosition());
		pWindow->GetCursorPos(cursorX, cursorY);
		if (flag)
		{
			pWindow->GetCursorPos(preCursorX, preCursorY);
			flag = false;
		}
		if (cursorX != preCursorX)
		{
			_parent->translate(
				glm::vec3((preCursorX - cursorX) * c, 0, 0), SceneNode::TransformSpace::LOCAL);
		}
		if (cursorY != preCursorY)
		{
			_parent->translate(
				glm::vec3(0, (cursorY - preCursorY) * c, 0), SceneNode::TransformSpace::LOCAL);
		}
		preCursorX = cursorX, preCursorY = cursorY;
	}
	else
	{
		flag = true;
	}
}
void EditCameraController::onWindow(Shit::Event const &event)
{
	if (auto p = std::get_if<Shit::MouseWheelEvent>(&event.value))
	{
		static float c = 0.1;
		_parent->translate(
			glm::vec3(0, 0, -glm::length(_parent->getGlobalPosition()) * c * p->yoffset), SceneNode::TransformSpace::LOCAL);
	}
}