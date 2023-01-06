#include "animation.hpp"
#include "root.hpp"
#include "buffer.hpp"
#include "descriptorManager.hpp"
#include "material.hpp"
#include "model.hpp"
#include "meshView.hpp"

DataType AnimationTrackChannel::getDataType(TransformPath transformPath)
{
	switch (transformPath)
	{
	case TransformPath::TRANSLATION: // vec3
	case TransformPath::SCALE:		 // vec3
		return DataType::VEC3;
	case TransformPath::ROTATION: // quat
		return DataType::VEC4;
	case TransformPath::WEIGTHS: // float
		return DataType::FLOAT;
	}
	return DataType::FLOAT;
}
AnimationTrackChannel::AnimationTrackChannel(
	AnimationNode *creator,
	TransformPath transformPath,
	InterpolationType interpolationType)
	: _creator(creator),
	  _transformPath(transformPath),
	  _interpolationType(interpolationType)
{
}

AnimationTrackChannel::AnimationTrackChannel(
	AnimationNode *creator,
	TransformPath transformPath,
	InterpolationType interpolationType,
	std::span<float> times,
	std::span<float> data)
	: _creator(creator),
	  _transformPath(transformPath),
	  _interpolationType(interpolationType),
	  _timeSequence(times.begin(), times.end()),
	  _data(data.begin(), data.end())
{
}
void AnimationTrackChannel::addKeyFrame(float time, float *data)
{
	auto count = getDataTypeSizeInBytes(getDataType(_transformPath)) / sizeof(float);
	auto it = _timeSequence.begin();
	auto end = _timeSequence.end();
	while (it != end && *it < time)
		++it;
	_data.insert(_data.begin() + (it - _timeSequence.begin()) * count, data, data + count);
	_creator->getCreator()->mergeKeyFrameTime(time);
}
std::vector<float> AnimationTrackChannel::getKeyFrameValue(std::span<float> pre, std::span<float> next, float factor)
{
	auto count = pre.size();
	std::vector<float> ret(count);
	switch (_interpolationType)
	{
	case InterpolationType::LINEAR:
	{
		if (_transformPath == TransformPath::ROTATION)
		{
			// wxyz
			auto a = glm::slerp(glm::quat(pre[3], pre[0], pre[1], pre[2]), glm::quat(next[3], next[0], next[1], next[2]), factor);
			// xyzw
			ret[0] = a[0];
			ret[1] = a[1];
			ret[2] = a[2];
			ret[3] = a[3];
		}
		else
		{
			for (size_t i = 0; i < count; ++i)
			{
				ret[i] = glm::mix(pre[i], next[i], factor);
			}
		}
	}
	break;
	case InterpolationType::STEP:
	{
		return {pre.begin(), pre.end()};
	}
	break;
	case InterpolationType::CUBICSPLINE:
	{
		LOG("cube spline not implemented yet")
		// for (size_t i = 0; i < count; ++i)
		//{
		//	//ret[i] = (pre[i], next[i], factor);
		// }
	}
	break;
	case InterpolationType::SPHERICAL_LINEAR:
	{
		LOG("SPHERICAL_LINEAR not implemented yet")
	}
	break;
	}
	return ret;
}
std::vector<float> AnimationTrackChannel::getKeyFrame(float time)
{
	auto count = _data.size() / _timeSequence.size();
	auto beg = _timeSequence.begin();
	auto it = beg;
	auto end = _timeSequence.end();
	if (it == end)
		return {};

	while (it != end && *it < time)
		++it;
	if (it == end)
	{
		auto dataIt = _data.end() - count;
		return std::vector<float>(dataIt, _data.end());
	}
	else if (it == beg)
	{
		auto dataIt = _data.begin();
		return std::vector<float>(dataIt, dataIt + count);
	}
	else
	{
		auto nextDataIt = _data.begin() + count * (it - beg);
		auto preDataIt = nextDataIt - count;
		auto preTimeIt = it - 1;
		auto factor = (time - *preTimeIt) / (*it - *preTimeIt);
		return getKeyFrameValue({preDataIt, preDataIt + count}, {nextDataIt, nextDataIt + count}, factor);
	}
}
//=====================
AnimationNode::AnimationNode(AnimationClip *creator) : _creator(creator)
{
	_name = "animationNode" + std::to_string(getId());
}
AnimationNode::AnimationNode(AnimationClip *creator, std::string_view name)
	: Node(name), _creator(creator)
{
}
AnimationNode::AnimationNode(AnimationNode const &other) : Node(other)
{
	_creator = other._creator;
	for (int i = 0; i < (int)TransformPath::Num; ++i)
		_channels[i] = std::make_unique<AnimationTrackChannel>(*other._channels[i]);
}
Node *AnimationNode::clone(bool recursive)
{
	auto ret = new AnimationNode(*this);
	if (recursive)
	{
		for (auto &&e : _children)
		{
			ret->addChild(e->clone(recursive));
		}
	}
	return ret;
}
AnimationTrackChannel *AnimationNode::createAnimationChannel(
	TransformPath transformPath,
	InterpolationType interpolationType,
	std::span<float> times,
	std::span<float> data)
{
	_creator->mergeKeyFrameTime(*times.rbegin());
	return (_channels[(size_t)transformPath] =
				std::make_unique<AnimationTrackChannel>(this, transformPath, interpolationType, times, data))
		.get();
}
AnimationTrackChannel *AnimationNode::getChannel(TransformPath transformPath) const
{
	if (_channels.at((size_t)transformPath))
		return _channels.at((size_t)transformPath).get();
	return nullptr;
}
AnimationTrackChannel *AnimationNode::createOrRetrieveAnimationChannel(
	TransformPath transformPath,
	InterpolationType interpolationType)
{
	if (auto p = getChannel(transformPath))
		return p;
	return createAnimationChannel(transformPath, interpolationType);
}
////==============================================
AnimationClip::AnimationClip()
{
	_name = "new animationclip" + std::to_string(getId());
	_animationRootNode = std::make_unique<AnimationNode>(this);
}
AnimationClip::AnimationClip(std::string_view name) : _name(name)
{
	_animationRootNode = std::make_unique<AnimationNode>(this);
}
//======================================
AnimationClipView::AnimationClipView(AnimationClip *clip, Node *rootNode) : _clip(clip)
{
	// apply animation to scenenode
	buildAnimationNodeMap(_clip->_animationRootNode.get(), rootNode);
}
void AnimationClipView::setRootNode(Node *rootNode)
{
	//_animationNodeMap.clear();
	_sceneNodeMap.clear();
	buildAnimationNodeMap(_clip->_animationRootNode.get(), rootNode);
}
void AnimationClipView::buildAnimationNodeMap(Node *animationNode, Node *sceneNode)
{
	//_animationNodeMap[animationNode] = sceneNode;
	_sceneNodeMap[sceneNode] = animationNode;

	auto it = animationNode->childBegin();
	auto end = animationNode->childEnd();
	while (it != end)
	{
		if (auto childNode = sceneNode->getChild((*it)->getName()))
		{
			buildAnimationNodeMap(it->get(), childNode);
		}
		++it;
	}
}
AnimationTrackChannel *AnimationClipView::createAnimationTrackChannel(
	Node *node,
	TransformPath transformPath,
	InterpolationType interpolationType,
	std::span<float> times,
	std::span<float> data)
{
	std::stack<Node *> stk;
	stk.push(node);
	while (stk.top() && !_sceneNodeMap.contains(stk.top()))
	{
		stk.push(stk.top()->getParent());
	}

	if (stk.top())
	{
		auto parentNode = stk.top();
		auto animationNode = _sceneNodeMap[parentNode];
		stk.pop();
		while (!stk.empty())
		{
			parentNode = stk.top();
			animationNode = animationNode->createChild(parentNode->getName(), false);
			_sceneNodeMap[parentNode] = animationNode;
			//_animationNodeMap[animationNode] = parentNode;
			stk.pop();
		}
		return static_cast<AnimationNode *>(animationNode)
			->createAnimationChannel(transformPath, interpolationType, times, data);
	}
	else
	{
		LOG("current node is not a childnode of current animation")
		return nullptr;
	}
}
void AnimationClipView::drive(float tSecond)
{
	for (auto e : _sceneNodeMap)
	{
		if (auto p = static_cast<AnimationNode *>(e.second)->getChannel(TransformPath::TRANSLATION))
		{
			auto vals = p->getKeyFrame(tSecond);
			static_cast<SceneNode *>(e.first)->setLocalTranslation({vals[0], vals[1], vals[2]});
		}
		if (auto p = static_cast<AnimationNode *>(e.second)->getChannel(TransformPath::SCALE))
		{
			auto vals = p->getKeyFrame(tSecond);
			static_cast<SceneNode *>(e.first)->setLocalScale({vals[0], vals[1], vals[2]});
		}
		if (auto p = static_cast<AnimationNode *>(e.second)->getChannel(TransformPath::ROTATION))
		{
			auto vals = p->getKeyFrame(tSecond);
			static_cast<SceneNode *>(e.first)->setLocalRotation({vals[3], vals[0], vals[1], vals[2]});
		}
		if (auto p = static_cast<AnimationNode *>(e.second)->getChannel(TransformPath::WEIGTHS))
		{
			auto vals = p->getKeyFrame(tSecond);
			std::vector<MeshView *> meshViews;
			static_cast<SceneNode *>(e.first)->getComponents(meshViews);
			for (auto meshView : meshViews)
			{
				meshView->setMorphWeights(vals);
			}
		}
	}
}
//===========================================
Animation::Animation(SceneNode *parent) : Behaviour(parent)
{
	_name = "Animation" + std::to_string(getId());
}
Animation::Animation(Animation const &other) : Behaviour(other)
{
	_curPlayingClipView = other._curPlayingClipView;
	_playAutomatically = other._playAutomatically;
	_wrapMode = other._wrapMode;
	_timer = other._timer;
	for (auto &&e : other._animationClipViews)
	{
		_animationClipViews[e.first] = std::make_unique<AnimationClipView>(*e.second);
	}
}
void Animation::setParentImpl(SceneNode *parent)
{
	for (auto &&e : _animationClipViews)
	{
		e.second->setRootNode(parent);
	}
}
AnimationClipView *Animation::addClip(AnimationClip *clip)
{
	return (_animationClipViews[std::string(clip->getName())] =
				std::make_unique<AnimationClipView>(clip, _parent))
		.get();
}
void Animation::removeClip(AnimationClip *clip)
{
	auto it = _animationClipViews.cbegin();
	auto end = _animationClipViews.cend();
	while (it != end)
	{
		if (it->second->getClip() == clip)
		{
			_animationClipViews.erase(it);
			return;
		}
	}
}
void Animation::removeClip(std::string_view name)
{
	_animationClipViews.erase(std::string(name));
}
bool Animation::isRunning()
{
	return _timer.IsRunning();
}
void Animation::play()
{
	_timer.Start();
}
void Animation::pause()
{
	_timer.Pause();
}
void Animation::stop()
{
	_timer.Stop();
	_curPlayingClipView = nullptr;
}
void Animation::setTime(float t_sec)
{
	_timer.SetTimeMs(t_sec * 1000);
	_curPlayingClipView->drive(t_sec);
}
void Animation::prepare()
{
	if (!_animationClipViews.empty())
	{
		_curPlayingClipView = _animationClipViews.begin()->second.get();
		_timer.Start();
	}
}
void Animation::update()
{
	if (_curPlayingClipView && _timer.IsRunning())
	{
		auto t = (float)_timer.getElapsedTimeInMs() / 1000.f;
		if (_wrapMode == WrapMode::LOOP)
		{
			auto dt = t - _curPlayingClipView->getClip()->getPeriod();
			if (dt > 0)
			{
				_timer.Restart();
				_curPlayingClipView->drive(dt);
				return;
			}
		}
		_curPlayingClipView->drive(t);
	}
}
//==================================================
void Joint::onNodeUpdated()
{
	_transformSignal(_jointIndex, _parent->getGlobalTransformMatrix() * _inverseBindingMatrix);
}
//==================================================
Skin::Skin()
{
	_name = "skin" + std::to_string(getId());
	prepareDescriptorSet();
}
Skin::Skin(std::string_view name) : _name(name)
{
	prepareDescriptorSet();
}
Skin::Skin(size_t jointCount, int *jointNodeIndices)
	: _jointNodeIndices(jointNodeIndices, jointNodeIndices + jointCount)
{
	_name = "skin" + std::to_string(getId());
}
Skin::Skin(std::string_view name, size_t jointCount, int *jointNodeIndices)
	: _name(name), _jointNodeIndices(jointNodeIndices, jointNodeIndices + jointCount)
{
}
Skin::Skin(Skin const &other) : IdObject<Skin>(other)
{
	_jointNodeIndices = other._jointNodeIndices;
}
void Skin::prepareDescriptorSet()
{
	//
	auto buffer = Root::getSingleton().getBufferManager()->createOrRetriveBuffer(
		BufferPropertyDesciption{
			Shit::BufferUsageFlagBits::STORAGE_BUFFER_BIT |
				Shit::BufferUsageFlagBits::TRANSFER_DST_BIT,
			Shit::MemoryPropertyFlagBits::HOST_VISIBLE_BIT |
				Shit::MemoryPropertyFlagBits::HOST_COHERENT_BIT,
			false});

	_gpuBufferView = buffer->allocate(sizeof(float) * (_jointNodeIndices.size() * 16 + 4), 1, 0);
	if (_jointNodeIndices.size() > 0)
	{
		_gpuBufferView.buffer->setData(_gpuBufferView.offset, sizeof(float), 1);
	}

	_descriptorSetData = Root::getSingleton().getDescriptorManager()->createDescriptorSetData(
		Root::getSingleton().getDescriptorSetLayoutSkin());

	_descriptorSetData->setBuffer(DESCRIPTOR_BINDING_SKIN, 0, _gpuBufferView);
}
void Skin::setOwner(Model *owner, int skinIndex)
{
	_owner = owner;

	for (auto i : _jointNodeIndices)
	{
		if (auto p = _owner->getChildByIndex(i))
		{
			std::vector<Joint *> joints;
			p->getComponents(joints);

			for (auto joint : joints)
			{
				if (joint->getSkinIndex() == skinIndex)
				{
					//joint->_model = owner;
					joint->addTransformListener(
						std::bind(&Skin::jointUpdateListener, this, std::placeholders::_1, std::placeholders::_2));
				}
			}
		}
	}
}
void Skin::prepare()
{
	prepareDescriptorSet();
}
void Skin::recoordCommandBuffer(Shit::CommandBuffer *cmdBuffer, uint32_t frameIndex)
{
	_descriptorSetData->upload(frameIndex);
	_descriptorSetData->bind(cmdBuffer,
							 Shit::PipelineBindPoint::GRAPHICS,
							 Root::getSingleton().getCommonPipelineLayout(), 
							 DESCRIPTORSET_SKIN,
							 frameIndex);
}
Skin *Skin::sGetEmptySkin()
{
	static Skin ret{};
	return &ret;
}
void Skin::jointUpdateListener(int jointIndex, glm::mat4 const &m)
{
	_gpuBufferView.buffer->setData(_gpuBufferView.offset + (4 + 16 * jointIndex) * sizeof(float), sizeof(float) * 16, &m);
}