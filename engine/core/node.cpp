#include "node.hpp"

Node::Node()
{
	_name = "node" + std::to_string(getId());
	//_signal(this, Event::CREATED);
}
Node::Node(std::string_view name) : _name(name)
{
	//_signal(this, Event::CREATED);
}
Node::Node(const Node &other)
{
	_name = other._name;
}
//void Node::copyChildNode(Node *childNode)
//{
//	_children.emplace_back(std::make_unique<Node>(*childNode));
//}
//Node &Node::operator=(const Node &other)
//{
//	_name = other._name;
//	return *this;
//}
Node::~Node()
{
	//_signal(this, Event::DESTROYED);
}
Node *Node::clone(bool recursive)
{
	auto ret = new Node(*this);
	if(recursive)
	{
		for (auto &&e : _children)
		{
			ret->addChild(e->clone(recursive));
		}
	}
	return ret;
}
Node *Node::getRoot() const
{
	auto node = this;
	while (node->_parent)
	{
		node = node->_parent;
	}
	return const_cast<Node *>(node);
}
void Node::prepare(bool prepareChildren)
{
	if (_needPrepare)
	{
		prepareImpl();
		_needPrepare = false;
	}
	if (prepareChildren)
	{
		for (auto&& p : _children)
			p->prepare(true);
	}
}
void Node::update(bool updateChildren, bool forceUpdateSelf)
{
	prepare();
	_parentNotified = false;
	if (_needSelfUpdate || forceUpdateSelf)
	{
		updateImpl();
		_needSelfUpdate = false;
		//_signal(this, Event::UPDATED);
	}
	if (updateChildren)
	{
		if (_needChildrenUpdate || forceUpdateSelf)
		{
			for (auto&& e : _children)
				e->update(true, true);
		}
		else
		{
			for (auto e : _childrenToUpdate)
				e->update(true, false);
		}
		_childrenToUpdate.clear();
		_needChildrenUpdate = false;
	}
}
void Node::needUpdate(bool notifyChildren, bool forceParentUpdate)
{
	_needSelfUpdate = true;
	_needChildrenUpdate = true;
	if (_parent && (!_parentNotified || forceParentUpdate))
	{
		_parent->requestUpdate(this, forceParentUpdate);
		_parentNotified = true;
	}
	// all children will be updated
	_childrenToUpdate.clear();

	if (notifyChildren)
	{
		for (auto &&p : _children)
		{
			p->_parentNotified = true;
			p->needUpdate(true, false);
		}
	}
}
void Node::requestUpdate(Node *child, bool forceParentUpdate)
{
	if (!_needChildrenUpdate)
		_childrenToUpdate.emplace(child);

	if (_parent && (!_parentNotified || forceParentUpdate))
	{
		_parent->requestUpdate(this, forceParentUpdate);
		_parentNotified = true;
	}
}
// void Node::cancelUpdate(const Node *child) const
//{
//	//_childrenToUpdate.erase(child);
//	//if (_childrenToUpdate.empty() && _parent && !_needChildrenUpdate)
//	//{
//	//	_parent->cancelUpdate(this);
//	//	_parentNotified = false;
//	//}
// }
void Node::setDepth(uint32_t newDepth)
{
	_depth = newDepth;
	for (auto &&p : _children)
		p->setDepth(newDepth + 1);
}
bool Node::addChild(Node *node)
{
	if (node->_parent)
	{
		LOG("node :", node->getId(), "already was a child of node", node->_parent->getId())
		return false;
	}
	node->_parent = this;
	_children.emplace_back(std::unique_ptr<Node>(node));
	node->setDepth(_depth + 1);
	node->needUpdate(true);
	addChildImpl(node);
	return true;
}
//Node *Node::createChild()
//{
//	auto node = createChildImpl();
//	addChild(node);
//	return node;
//}
Node *Node::createChild(std::string_view name, bool isStatic)
{
	auto node = createChildImpl(name, isStatic);
	addChild(node);
	return node;
}
Node *Node::getChild(int index) const
{
	return _children.at(index).get();
}
Node *Node::getChild(std::string_view name) const
{
	auto it = ranges::find_if(_children, [&name](auto &&e)
							  { return e->getName() == name; });
	if (it == _children.cend())
		return nullptr;
	return it->get();
}
void Node::removeChild(ConstChildNodeIter it)
{
	(*it)->_parent = nullptr;
	_children.erase(it);
}
void Node::removeChild(int index)
{
	removeChild(_children.begin() + index);
}
void Node::removeChild(Node *node)
{
	auto it = std::find_if(_children.begin(), _children.end(), [node](auto &&e)
						   { return e.get() == node; });
	if (it != _children.end())
		removeChild(it);
}
void Node::removeAllChildren()
{
	_children.clear();
}