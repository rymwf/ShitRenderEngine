#pragma once
#include "prerequisites.hpp"
#include "idObject.hpp"
#include "component.hpp"
#include "timer.hpp"
#include "behaviour.hpp"
#include "sceneNode.hpp"

/**
 * @brief Determines how time is treated outside of the keyframed range
 *
 */
enum WrapMode
{
	ONCE,		   // When time reaches the end of the animation clip, the clip will automatically stop playing and time will be reset to beginning of the clip.
	LOOP,		   // When time reaches the end of the animation clip, time will continue at the beginning.
	PINGPONG,	   // When time reaches the end of the animation clip, time will ping pong back between beginning and end.
	DEFAULT,	   // Reads the default repeat mode set higher up.
	CLAMP_FOREVER, // Plays back the animation. When it reaches the end, it will keep playing the last frame and never stop playing.
};
enum class TransformPath
{
	TRANSLATION, // vec3
	ROTATION,	 // quat, xyzw
	SCALE,		 // vec3
	WEIGTHS,	 // float
	Num
};
enum class InterpolationType
{
	STEP,			  // vt=vk
	LINEAR,			  // vt=(1-t)*vk+t*v(k+1) vk=mix(vk,vk+1,t)
	SPHERICAL_LINEAR, // vt=sin(a(1-t))/sin(a)*vk+s*sin(at)/sin(a)*vk+1
	CUBICSPLINE,	  // vt=smoothstep(vk,vk+1,tvt=(2t3-3t2+1)*vk+td(t3-2t2+t)*bk+(-2t3+3t2)*vk+1+td(t3-t2)*ak+1
};
class AnimationNode;
class AnimationClip;

class AnimationTrackChannel : public IdObject<AnimationTrackChannel>
{
	AnimationNode *_creator;
	InterpolationType _interpolationType = InterpolationType::LINEAR;

	TransformPath _transformPath{};
	/**
	 * @brief time unit is second
	 *
	 */
	std::vector<float> _timeSequence{};
	std::vector<float> _data{};

	/**
	 * @brief Get the Key Frame Value object
	 *
	 * @param pre
	 * @param next
	 * @param factor [0-1]
	 * @return std::vector<float>
	 */
	std::vector<float> getKeyFrameValue(std::span<float> pre, std::span<float> next, float factor);

public:
	static DataType getDataType(TransformPath transformPath);

	AnimationTrackChannel(
		AnimationNode *creator,
		TransformPath transformPath,
		InterpolationType interpolationType = InterpolationType::LINEAR);

	AnimationTrackChannel(
		AnimationNode *creator,
		TransformPath transformPath,
		InterpolationType interpolationType,
		std::span<float> times,
		std::span<float> data);

	virtual ~AnimationTrackChannel() {}

	constexpr AnimationNode *getCreator() const { return _creator; }

	constexpr TransformPath getTransformPath() const { return _transformPath; }

	void addKeyFrame(float time, float *data);

	std::vector<float> getKeyFrame(float time);

	constexpr AnimationTrackChannel *setInterpolationType(InterpolationType interpolationType)
	{
		_interpolationType = interpolationType;
		return this;
	}
};

class AnimationNode : public Node
{
	std::array<std::unique_ptr<AnimationTrackChannel>, (size_t)TransformPath::Num> _channels{};

	AnimationClip *_creator;

	int _jointIndex{-1};

	Node *createChildImpl(std::string_view name, bool isStatic) override
	{
		return new AnimationNode(_creator, name);
	}
	Node *createChildImpl() override
	{
		return new AnimationNode(_creator);
	}

public:
	AnimationNode(AnimationClip *creator);
	AnimationNode(AnimationClip *creator, std::string_view name);

	AnimationNode(AnimationNode const &other);

	~AnimationNode() {}

	NODISCARD Node *clone(bool recursive = true);

	constexpr AnimationClip *getCreator() const { return _creator; }

	AnimationTrackChannel *createAnimationChannel(
		TransformPath transformPath,
		InterpolationType interpolationType,
		std::span<float> times = {},
		std::span<float> data = {});

	AnimationTrackChannel *createOrRetrieveAnimationChannel(
		TransformPath transformPath,
		InterpolationType interpolationType = InterpolationType ::LINEAR);

	AnimationTrackChannel *getChannel(TransformPath transformPath) const;
};

class AnimationClip : public IdObject<AnimationClip>
{
	friend class AnimationClipView;
	std::unique_ptr<AnimationNode> _animationRootNode{};

	float _timePeriod{};

	std::string _name;

	void init();

public:
	AnimationClip();
	AnimationClip(std::string_view name);

	constexpr std::string_view getName() const { return _name; }
	constexpr float getPeriod() const { return _timePeriod; }

	void mergeKeyFrameTime(float frameTime)
	{
		_timePeriod = std::max(_timePeriod, frameTime);
	}

	AnimationNode *getRootNode() const { return _animationRootNode.get(); }
};

class AnimationClipView
{
	AnimationClip *_clip;

	// key is animation node, val is scenenode
	// std::unordered_map<Node *, Node *> _animationNodeMap;

	// key is scenenode, val is animation node
	std::unordered_map<Node *, Node *> _sceneNodeMap;

	void buildAnimationNodeMap(Node *animationNode, Node *sceneNode);

public:
	AnimationClipView(AnimationClip *clip, Node *rootNode);

	void setRootNode(Node *rootNode);

	/**
	 * @brief Create animationtrackChannel for a scenenode
	 *
	 * @param node  node must be child of rootnode
	 * @return AnimationTrackChannel*
	 */
	AnimationTrackChannel *createAnimationTrackChannel(
		Node *node,
		TransformPath transformPath,
		InterpolationType interpolationType,
		std::span<float> times = {},
		std::span<float> data = {});

	constexpr AnimationClip *getClip() const { return _clip; }

	virtual ~AnimationClipView() {}

	void drive(float tSecond);
};

class Animation : public Behaviour
{
public:
	using ClipViewContainer = std::unordered_map<std::string, std::unique_ptr<AnimationClipView>>;
	using ClipViewContainerIter = ClipViewContainer::iterator;
	using ClipViewContainerCIter = ClipViewContainer::const_iterator;

	Animation(SceneNode *parent);
	Animation(Animation const &other);

	NODISCARD Component *clone() override
	{
		return new Animation(*this);
	}
	constexpr Timer const &getTimer() const { return _timer; }

	AnimationClipView *addClip(AnimationClip *clip);
	void removeClip(AnimationClip *clip);
	void removeClip(std::string_view name);

	ClipViewContainerIter clipViewBegin() { return _animationClipViews.begin(); }
	ClipViewContainerIter clipViewEnd() { return _animationClipViews.end(); }
	ClipViewContainerCIter clipViewCbegin() const { return _animationClipViews.cbegin(); }
	ClipViewContainerCIter clipViewCend() const { return _animationClipViews.cend(); }

	AnimationClipView *getClipView(std::string_view name) const
	{
		return _animationClipViews.at(std::string(name)).get();
	}
	void setCurClipView(AnimationClipView *clipView)
	{
		_curPlayingClipView = clipView;
	}
	void setCurClipView(std::string_view name)
	{
		_curPlayingClipView = getClipView(name);
	}
	constexpr AnimationClipView *getCurClipView() const
	{
		return _curPlayingClipView;
	}

	constexpr void setWrapMode(WrapMode wrapMode) { _wrapMode = wrapMode; }
	constexpr WrapMode getWrapMode() const { return _wrapMode; }
	constexpr bool isPlayAutomatically() const { return _playAutomatically; }
	constexpr void setPlayAutomatically(bool val) { _playAutomatically = val; }

	bool isRunning();

	void play();
	void pause();
	void stop();
	void setTime(float t_sec);

	// float getTime() const { return _timer.getElapsedTimeInMs() / 1000; }
	float getPeriod() const { return _curPlayingClipView->getClip()->getPeriod(); }

	void prepare() override;
	void update() override;

private:
	ClipViewContainer _animationClipViews;
	AnimationClipView *_curPlayingClipView;

	bool _playAutomatically{true};

	WrapMode _wrapMode{WrapMode::LOOP};

	Timer _timer{};

	void setParentImpl(SceneNode *parent) override;
};

class Joint : public Component
{
	int _skinIndex;
	int _jointIndex;
	glm::mat4 _inverseBindingMatrix;

	Shit::Signal<void(int, glm::mat4 const &)> _transformSignal;

public:
	Joint(SceneNode *parent, int jointIndex, glm::mat4 const &inverseBindingMatrix, int skinIndex = 0)
		: Component(parent),
		  _skinIndex(skinIndex),
		  _jointIndex(jointIndex),
		  _inverseBindingMatrix(inverseBindingMatrix)
	{
		_name = "joint" + std::to_string(getId());
	}
	Joint(Joint const &other) : Component(other)
	{
		_skinIndex = other._skinIndex;
		_jointIndex = other._jointIndex;
		_inverseBindingMatrix = other._inverseBindingMatrix;
	}

	constexpr int getSkinIndex() const { return _skinIndex; }

	void addTransformListener(Shit::Slot<void(int, glm::mat4 const &)> const &slot)
	{
		_transformSignal.Connect(slot);
	}
	void removeTransformListener(Shit::Slot<void(int, glm::mat4 const &)> const &slot)
	{
		_transformSignal.Disconnect(slot);
	}
	void onNodeUpdated() override;

	NODISCARD Component *clone() override
	{
		return new Joint(*this);
	}
};

class Skin : public IdObject<Skin>
{
	std::string _name;
	Model *_owner{};
	/**
	 * @brief
	 *
layout(std430,binding=0 SET(2)) buffer UBOSkin
{
	int hasSkin;
	mat4 jointMatrices[];
};
	 *
	 */
	// std::vector<float> _uboData;
	std::vector<int> _jointNodeIndices;

	BufferView _gpuBufferView;

	DescriptorSetData *_descriptorSetData;

	void prepareDescriptorSet();

public:
	Skin();
	Skin(std::string_view name);
	Skin(size_t jointCount, int *jointNodeIndices);
	Skin(std::string_view name, size_t jointCount, int *jointNodeIndices);
	Skin(Skin const &other);

	constexpr std::string_view getName() const { return _name; }

	void setOwner(Model *owner, int skinIndex);

	void recoordCommandBuffer(Shit::CommandBuffer *cmdBuffer, uint32_t frameIndex);

	void jointUpdateListener(int jointIndex, glm::mat4 const &m);

	void prepare();

	static Skin *sGetEmptySkin();
};
