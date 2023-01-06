#pragma once
#include "prerequisites.hpp"
#include "idObject.hpp"

class Node : public IdObject<Node>
{
public:
	enum class Event
	{
		CREATED,
		PARENT_CHANGED,
		UPDATED,
		DESTROYED,
	};
	using ChildNodeContainer = std::vector<std::unique_ptr<Node>>;
	using ConstChildNodeIter = ChildNodeContainer::const_iterator;
	using ChildNodeIter = ChildNodeContainer::iterator;

	Node();
	Node(std::string_view name);
	/**
	 * @brief only copy current node, do not include children
	 * 		and the parent node is empty
	 *
	 * @param other
	 */
	Node(const Node &other);
	//Node &operator=(const Node &other);

	virtual ~Node();

	virtual NODISCARD Node *clone(bool recursive = true);

	bool addChild(Node *node);

	//Node *createChild();
	Node *createChild(std::string_view name, bool isStatic);

	Node *getChild(int index) const;
	Node *getChild(std::string_view name) const;

	Node *getRoot() const;

	constexpr ConstChildNodeIter childCbegin() const { return _children.cbegin(); }
	constexpr ConstChildNodeIter childCend() const { return _children.cend(); }
	constexpr ChildNodeIter childBegin() { return _children.begin(); }
	constexpr ChildNodeIter childEnd() { return _children.end(); }

	/**
	 * @brief
	 *
	 * @param index child index in container
	 */
	void removeChild(int index);
	void removeChild(Node *node);
	void removeChild(ConstChildNodeIter it);
	void removeAllChildren();

	constexpr int childrenNum() const { return _children.size(); }

	constexpr void setName(std::string_view name) { _name = name; }

	constexpr std::string_view getName() const { return _name; }

	Node *getParent() const { return _parent; }

	/**
	 * @brief update self node
	 *
	 * @param updateChildren if true, update children those need to be updated
	 * @param forceUpdateSelf force update self and  if updateChildren is true, update all children
	 */
	void update(bool updateChildren, bool forceUpdateSelf = false);

	/**
	 * @brief only prepare self
	 *
	 */
	void prepare(bool prepareChildren = false);

	/**
	 * @brief set all children need to be updated and tell parent node this node need to be updated
	 *
	 * @param forceParentUpdate tell parent again
	 */
	void needUpdate(bool notifyChildren = true, bool forceParentUpdate = false);

	///**
	// * @brief do not update the child
	// *
	// * @param child
	// */
	// void cancelUpdate(const Node *child) const;

	virtual std::string getTypeName() const { return NAME_NODE; }

	constexpr uint32_t getDepth() const { return _depth; }

protected:
	void setDepth(uint32_t newDepth);

	//virtual void copyChildNode(Node *child);

	virtual void updateImpl() {}

	virtual void addChildImpl(Node *node) {}

	virtual Node *createChildImpl(std::string_view name, bool isStatic)
	{
		return new Node(name);
	}
	virtual Node *createChildImpl()
	{
		return new Node();
	}
	virtual void prepareImpl() {}

	std::string _name;
	ChildNodeContainer _children;

	// used if self is not outof date but chilren are
	mutable std::unordered_set<Node *> _childrenToUpdate;
	Node *_parent{};

	uint32_t _depth{};

	mutable bool _needSelfUpdate{true};		// currentNode need to be updated
	mutable bool _needChildrenUpdate{true}; // all children need to be updated
	mutable bool _parentNotified{false};	// tell parent this node needs tobe updated
	mutable bool _needPrepare{true};

	// mutable Shit::Signal<void(Node const *, Event)> _signal;
	//  mutable Shit::Signal<void(Node const *, MovableObject const *, bool attached)> _movableObjectAttachmentSignal;

	/**
	 * @brief
	 *
	 * @param child this child need to be update
	 * @param forceParentUpdate tell parent this node need to be updated
	 */
	void requestUpdate(Node *child, bool forceParentUpdate = false);
};

template <typename _Pred>
void traverseNodeDF(Node *node, _Pred pred)
{
	pred(node);
	auto it = node->childBegin();
	auto end = node->childEnd();
	for (; it != end; ++it)
	{
		traverseNodeDF(it->get(), pred);
	}
}
template <typename _Pred>
void traverseNodeBF(Node *node, _Pred pred)
{
	std::queue<Node *> q;
	q.push(node);
	while (!q.empty())
	{
		node = q.front();
		pred(node);
		q.pop();
		auto it = node->childBegin();
		auto end = node->childEnd();
		for (; it != end; ++it)
		{
			q.push(it->get());
		}
	}
}
