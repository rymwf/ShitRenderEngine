#pragma once
#include "node.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "renderable.hpp"
#include "light.hpp"

struct Transform
{
	glm::vec3 translation;
	glm::vec3 scale{1, 1, 1};
	glm::quat rotation{1, 0, 0, 0}; // wxyz
									// glm::mat4 matrix;
};

//enum class SceneNodeType
//{
//	SCENENODE,
//	MODEL,
//};

class SceneNode : public Node
{
public:
	enum class TransformSpace
	{
		LOCAL,
		PARENT,
		WORLD
	};
	using ComponentContainer = std::vector<std::unique_ptr<Component>>;
	using ComponentIter = ComponentContainer::iterator;
	using ComponentCIter = ComponentContainer::const_iterator;

	SceneNode(bool isStatic, Scene *scene = nullptr);
	SceneNode(std::string_view name, bool isStatic, Scene *scene = nullptr);

	SceneNode(SceneNode const &other);
	//SceneNode &operator=(SceneNode const &other);

	~SceneNode() {}

	NODISCARD Node *clone(bool recursive = true) override;

	/**
	 * @brief recursively
	 * 
	 * @param flag 
	 */
	void enable(bool flag);
	constexpr bool isEnable() const { return _enable; }

	// constexpr SceneNode *inheritRotation(bool enable)
	//{
	//	_inheritRotation = enable;
	//	return this;
	// }
	// constexpr SceneNode *inheritScale(bool enable)
	//{
	//	_inheritScale = enable;
	//	return this;
	// }

	constexpr const Transform &getInitialTransform() const { return _initialTransform; }
	constexpr const Transform &getLocalTransform() const { return _localTransform; }
	constexpr glm::mat4 const &getGlobalTransformMatrix() const { return _globalTransformMatrix; }
	constexpr glm::vec3 getGlobalPosition() const { return glm::vec3(getGlobalTransformMatrix()[3]); }

	/**
	 * @brief Set current state as Initial State
	 *
	 */
	void setInitialState(const Transform transform);

	void resetToInitialState();

	void setLocalTransform(const Transform &transform);
	void setLocalTranslation(glm::vec3 const &translation);
	void setLocalScale(glm::vec3 const &scale);
	void setLocalRotation(glm::quat const &rotation);

	SceneNode *translate(const glm::vec3 d, TransformSpace space = TransformSpace::PARENT);
	SceneNode *rotate(const glm::quat &auat, TransformSpace space = TransformSpace::PARENT);
	SceneNode *scale(const glm::vec3 &scale, TransformSpace space = TransformSpace::LOCAL);

	// roll yaw pitch is working in local space
	SceneNode *roll(float radian, TransformSpace space = TransformSpace::PARENT)
	{
		return rotate(glm::quat(glm::vec3(0, 0, radian)), space);
	}
	SceneNode *yaw(float radian, TransformSpace space = TransformSpace::PARENT)
	{
		return rotate(glm::quat(glm::vec3(0, radian, 0)), space);
	}
	SceneNode *pitch(float radian, TransformSpace space = TransformSpace::PARENT)
	{
		return rotate(glm::quat(glm::vec3(radian, 0, 0)), space);
	}

	//============================================
	// components
	constexpr size_t getComponentSize() const { return _components.size(); }

	void addComponent(Component *component);

	void removeComponent(Component *component);

	template <typename T, typename... Args>
	T *addComponent(Args... args)
	{
		auto p = new T(this, args...);
		addComponent(p);
		return p;
	}

	/**
	 * @brief Get the Components object
	 *
	 * @tparam T
	 * @param components
	 * @return size_t number of components found
	 */
	template <typename T>
	void getComponents(std::vector<T *> &components, bool recursive = false) const
	{
		if (recursive)
		{
			getComponentsHelper(this, components);
		}
		else
		{
			for (auto &&component : _components)
			{
				if (auto p = dynamic_cast<T *>(component.get()))
				{
					components.emplace_back(p);
				}
			}
		}
	}
	void getComponents(std::vector<Component *> &components, bool recursive = false) const;
	Component *getComponentByIndex(size_t index) const { return _components.at(index).get(); }

	constexpr Scene *getParentScene() const { return _scene; }

	constexpr DescriptorSetData *getDescriptorSetData() const { return _descriptorSetData; }

	void enableDescriptorSet() { _createDescriptorSet = true; }

	virtual void renderableEventListener(Renderable *renderable, Renderable::Event event);

protected:
	//SceneNodeType _sceneNodeType{SceneNodeType::SCENENODE};

	Scene *_scene = nullptr; //parent scene

	bool _enable = true;
	bool _static = false; //if true does not move at run time, and it can be precomputed , not used currently

	// bool _needGlobalTransformMatrixUpdate{true};
	bool _inheritRotation{true};
	bool _inheritScale{true};

	uint32_t _layerMask = 1;

	Transform _initialTransform;
	Transform _localTransform;

	glm::mat4 _globalTransformMatrix{1};

	//create when request
	bool _createDescriptorSet{false};
	BufferView _transformBufferView{};
	DescriptorSetData *_descriptorSetData{};

	std::vector<std::unique_ptr<Component>> _components;

	// Shit::Signal<void(glm::mat4 const &)> _transformSignal;
protected:
	friend class Component;
	friend class Model;
	friend class Scene;

	void prepareDescriptorSet();

	ComponentIter componentBegin() { return _components.begin(); }
	ComponentIter componentEnd() { return _components.end(); }

	ComponentCIter componentCbegin() const { return _components.cbegin(); }
	ComponentCIter componentCend() const { return _components.cend(); }

	void init();

	void setScene(Scene *scene);

	void addChildImpl(Node *node) override;

	Node *createChildImpl(std::string_view name, bool isStatic) override
	{
		if (name.empty())
			return new SceneNode(isStatic);
		return new SceneNode(name, isStatic);
	}

	void prepareImpl() override;
	void updateImpl() override;

	//virtual void preRenderImpl(uint32_t frameIndex);
	//virtual void postRenderImpl(uint32_t frameIndex);

	template <typename T>
	void getComponentsHelper(SceneNode const *node, std::vector<T *> &components) const
	{
		for (auto &&component : node->_components)
		{
			if (auto p = dynamic_cast<T *>(component.get()))
			{
				components.emplace_back(p);
			}
		}
		for (auto &&p : node->_children)
		{
			getComponentsHelper(static_cast<SceneNode const *>(p.get()), components);
		}
	}
	void getComponentsHelper(SceneNode const *node, std::vector<Component *> &components) const;
};