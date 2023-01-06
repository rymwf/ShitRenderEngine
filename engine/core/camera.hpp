#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <variant>
#include "component.hpp"
#include "viewLayer.hpp"

struct CameraPara
{
	alignas(16) glm::mat4 V;
	alignas(16) glm::mat4 P;
	glm::vec3 eyePos;
	int cascadeNum;
	float cascadeSplit[8];
};

struct PerspectiveDescription
{
	float fovy;	  // radians
	float aspect; // width/height
};
struct OrthogonalDescription
{
	float xmag;
	float ymag;
};
class Camera : public Component
{
	float _near;
	float _far; // if far==0, means infinite perspective, orthogonal cannot be 0
	std::variant<PerspectiveDescription, OrthogonalDescription> _extraDesc;

	glm::mat4 _perspectiveMatrix;

	Shit::ClearColorValueFloat _clearColorValue;

	float _focalLength{0.05}; // m
	float _cameraSize{0.036}; // m

	// range [0,1]
	Rect2D<float> _relativeViewport{{0, 0}, {1, 1}};
	Rect2D<float> _relativeScissor{{0, 0}, {1, 1}};

	int _cullingMask = 1;

	//========================
	CameraPara _cameraPara;
	BufferView _gpuBufferView;
	IdType _setDataId;

	//================
	std::unique_ptr<ViewLayer> _viewLayer;

	RenderPath _renderPath{RenderPath::DEFERRED};

	std::unique_ptr<RenderQueueGroup> _renderQueues;

	std::vector<Renderable *> _visibleRenderables;
	std::vector<Light *> _lights;

	std::array<glm::vec3, 4> _nearVerticesInCameraSpace;
	std::array<glm::vec3, 4> _farVerticesInCameraSpace;

	std::vector<float> _cascadeSplitRatios;

	Shit::Signal<void(Camera const *)> _updataSignal;

	void updateProjectionMatrix();

	void prepareDescriptorSet();

	void cullScene();

public:
	Camera(SceneNode *parent);
	//Camera(SceneNode *parent, PerspectiveDescription perspectiveDesc, float n = 0.1f, float f = 1000.f);
	//Camera(SceneNode *parent, OrthogonalDescription orthogonalDesc, float n = 0.1f, float f = 1000.f);
	//Camera(SceneNode *parent, float aspect, float n, float f);
	Camera(Camera const &other);

	~Camera();

	NODISCARD Component *clone() override
	{
		return new Camera(*this);
	}

	DescriptorSetData *getDescriptorSetData() const;

	void render(uint32_t frameIndex);

	Camera *setOrthogonal(OrthogonalDescription orth = {8, 6});
	Camera *setPerspective(PerspectiveDescription persp = {45.f, 4.f / 3.f});
	Camera *setNearPlane(float near);
	Camera *setFarPlane(float far);

	Camera *setViewportAspect(float aspect);
	Camera *setFocalLength(float focalLength);
	Camera *setCameraSize(float cameraSize);

	void setRenderPath(RenderPath renderPath);
	constexpr RenderPath getRenderPath() const { return _renderPath; }

	constexpr float getNearPlane() const { return _near; }
	constexpr float getFarPlane() const { return _far; }
	constexpr bool isOrthogonal() const
	{
		return std::holds_alternative<OrthogonalDescription>(_extraDesc);
	}
	PerspectiveDescription const *getPerspectiveDescription() const
	{
		return std::get_if<PerspectiveDescription>(&_extraDesc);
	}
	OrthogonalDescription const *getOrthogonalDescription() const
	{
		return std::get_if<OrthogonalDescription>(&_extraDesc);
	}

	glm::mat4 getViewMatrix() const;
	constexpr glm::mat4 const &getProjectionMatrix() const
	{
		return _perspectiveMatrix;
	}

	constexpr void setClearColorValue(Shit::ClearColorValueFloat const &clearColorValue)
	{
		_clearColorValue = clearColorValue;
	}
	constexpr Shit::ClearColorValueFloat const &getClearColorValue() const { return _clearColorValue; }

	ViewLayer *getViewLayer() const
	{
		return _viewLayer.get();
	}
	constexpr Rect2D<float> const &getRelativeViewport() const { return _relativeViewport; }
	constexpr Rect2D<float> const &getRelativeScissor() const { return _relativeScissor; }

	constexpr void setRelativeViewport(Rect2D<float> const &ext)
	{
		_relativeViewport = ext;
	}
	constexpr void setRelativeScissor(Rect2D<float> const &ext)
	{
		_relativeScissor = ext;
	}

	void setViewLayer(Shit::Extent2D ext, Shit::Format format);

	void prepare() override;
	/**
	 * @brief called by scenenode
	 *
	 */
	void onNodeUpdated() override;
	void updateGPUData(uint32_t frameIndex) override;

	void setCascadeSplitRatios(std::span<float const> ratios);
	std::array<glm::vec3, 8> getFrustumVertices(int cascadeIndex) const;
	constexpr int getCascadeNum() const { return _cascadeSplitRatios.size() + 1; }

	void addUpdateListener(Shit::Slot<void(Camera const *)> slot)
	{
		_updataSignal.Connect(slot);
	}
	void removeUpdateListener(Shit::Slot<void(Camera const *)> slot)
	{
		_updataSignal.Disconnect(slot);
	}
};