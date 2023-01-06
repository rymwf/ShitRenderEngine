#include "camera.hpp"
#include "root.hpp"
#include "sceneManager.hpp"
#include "texture.hpp"
#include "light.hpp"

Camera::Camera(SceneNode *parent)
	: Component(parent), _clearColorValue(Shit::ClearColorValueFloat{}),
	  _near(0.1f), _far(200.f)
{
	_name = "Camera" + std::to_string(getId());
	_renderQueues = std::make_unique<RenderQueueGroup>(this);
	auto screen = Root::getSingleton().getScreen();
	auto &&ext = screen->getFramebufferSize();
	setViewLayer(ext, Shit::Format::R8G8B8A8_SRGB);
	auto aspect = float(ext.width) / ext.height;

	_extraDesc = PerspectiveDescription{
		float(2.f * std::atan2(1. / (2 * aspect), _focalLength / _cameraSize)),
		aspect};

	float splits[]{0.1, 0.3, 0.5};
	setCascadeSplitRatios(splits);
}
// Camera::Camera(SceneNode *parent, PerspectiveDescription perspectiveDesc, float n, float f)
//	: Component(parent), _extraDesc(perspectiveDesc), _near(n), _far(f)
//{
//	_renderQueues = std::make_unique<RenderQueueGroup>(this);
//	auto screen = Root::getSingleton().getScreen();
//	setViewLayer(screen->getFramebufferSize(), Shit::Format::R8G8B8A8_SRGB);
// }
// Camera::Camera(SceneNode *parent, OrthogonalDescription orthogonalDesc, float n, float f)
//	: Component(parent), _extraDesc(orthogonalDesc), _near(n), _far(f)
//{
//	_renderQueues = std::make_unique<RenderQueueGroup>(this);
//	auto screen = Root::getSingleton().getScreen();
//	setViewLayer(screen->getFramebufferSize(), Shit::Format::R8G8B8A8_SRGB);
// }
// Camera::Camera(SceneNode *parent, float aspect, float n, float f)
//	: Component(parent), _near(n), _far(f),
//	  _extraDesc(PerspectiveDescription{float(2.f * std::atan2(1. / (2 * aspect), 0.05 / _cameraSize)), aspect})
//{
//	_renderQueues = std::make_unique<RenderQueueGroup>(this);
//	auto screen = Root::getSingleton().getScreen();
//	setViewLayer(screen->getFramebufferSize(), Shit::Format::R8G8B8A8_SRGB);
// }
Camera::Camera(Camera const &other) : Component(other)
{
	_near = other._near;
	_far = other._far;
	_extraDesc = other._extraDesc;
	_perspectiveMatrix = other._perspectiveMatrix;
	_cullingMask = other._cullingMask;
	_focalLength = other._focalLength;
	_cameraSize = other._cameraSize;

	prepareDescriptorSet();
}
Camera::~Camera()
{
	// Root::getSingleton().getDescriptorManager()->remove(_setDataId);
}
void Camera::prepareDescriptorSet()
{
	auto buffer = Root::getSingleton().getBufferManager()->createOrRetriveBuffer(
		BufferPropertyDesciption{
			Shit::BufferUsageFlagBits::STORAGE_BUFFER_BIT,
			Shit::MemoryPropertyFlagBits::HOST_VISIBLE_BIT |
				Shit::MemoryPropertyFlagBits::HOST_COHERENT_BIT,
			true});

	_cameraPara.P = getProjectionMatrix();
	_cameraPara.V = getViewMatrix();
	_cameraPara.eyePos = _parent->getGlobalPosition();

	_gpuBufferView = buffer->allocate(sizeof(CameraPara), 1, &_cameraPara);

	// create camera descriptor set
	auto setData = Root::getSingleton().getDescriptorManager()->createDescriptorSetData(
		Root::getSingleton().getDescriptorSetLayoutCamera(), true);

	_setDataId = setData->getId();
	setData->setBuffer(DESCRIPTOR_BINDING_CAMERA, 0, _gpuBufferView);
}
DescriptorSetData *Camera::getDescriptorSetData() const
{
	return static_cast<DescriptorSetData *>(Root::getSingleton().getDescriptorManager()->get(_setDataId));
}
glm::mat4 Camera::getViewMatrix() const
{
	return glm::inverse(_parent->getGlobalTransformMatrix());
}
Camera *Camera::setOrthogonal(OrthogonalDescription orth)
{
	_extraDesc = orth;
	_parent->needUpdate(false);
	return this;
}
Camera *Camera::setPerspective(PerspectiveDescription persp)
{
	_extraDesc = persp;
	_parent->needUpdate(false);
	return this;
}
Camera *Camera::setNearPlane(float near)
{
	_near = near;
	_parent->needUpdate(false);
	return this;
}
Camera *Camera::setFarPlane(float far)
{
	_far = far;
	_parent->needUpdate(false);
	return this;
}
Camera *Camera::setViewportAspect(float aspect)
{
	if (auto p = std::get_if<PerspectiveDescription>(&_extraDesc))
	{
		p->aspect = aspect;
	}
	else if (auto p2 = std::get_if<OrthogonalDescription>(&_extraDesc))
	{
		p2->ymag = p2->xmag / aspect;
	}
	_parent->needUpdate(false);
	return this;
}
Camera *Camera::setCameraSize(float cameraSize)
{
	_cameraSize = cameraSize;
	if (auto p = std::get_if<PerspectiveDescription>(&_extraDesc))
	{
		p->fovy = float(2.f * std::atan2(1. / (2 * p->aspect), _focalLength / _cameraSize));
	}
	_parent->needUpdate(false);
	return this;
}
Camera *Camera::setFocalLength(float focalLength)
{
	_focalLength = focalLength;
	if (auto p = std::get_if<PerspectiveDescription>(&_extraDesc))
	{
		p->fovy = float(2.f * std::atan2(1. / (2 * p->aspect), _focalLength / _cameraSize));
	}
	_parent->needUpdate(false);
	return this;
}
void Camera::setRenderPath(RenderPath renderPath)
{
	_renderPath = renderPath;
}

void Camera::updateProjectionMatrix()
{
	if (auto p = std::get_if<PerspectiveDescription>(&_extraDesc))
	{
		_perspectiveMatrix = _far
								 ? glm::perspective(p->fovy, p->aspect, _near, _far)
								 : glm::infinitePerspective(p->fovy, p->aspect, _near);

		auto tanHalfFovy = glm::tan(p->fovy / 2);
		auto yn = tanHalfFovy * _near;
		auto yf = tanHalfFovy * _far;
		auto xn = yn * p->aspect;
		auto xf = yf * p->aspect;

		_nearVerticesInCameraSpace = {
			glm::vec3{-xn, -yn, -_near},
			glm::vec3{xn, -yn, -_near},
			glm::vec3{-xn, yn, -_near},
			glm::vec3{xn, yn, -_near},
		};
		_farVerticesInCameraSpace = {
			glm::vec3{-xf, -yf, -_far},
			glm::vec3{xf, -yf, -_far},
			glm::vec3{-xf, yf, -_far},
			glm::vec3{xf, yf, -_far},
		};
	}
	else
	{
		auto p2 = std::get_if<OrthogonalDescription>(&_extraDesc);
		auto x = p2->xmag / 2;
		auto y = p2->ymag / 2;

		// ret = glm::ortho(-p2->xmag / 2, p2->xmag / 2, -p2->ymag / 2, p2->ymag / 2, n, f);
		// TODO: need to detect depth range
		_perspectiveMatrix = glm::orthoRH_ZO(-x, x, -y, y, _near, _far);
		// _perspectiveMatrix = glm::orthoRH_ZO(-p2->xmag / 2, p2->xmag / 2, -p2->ymag / 2, p2->ymag / 2, _near, _far);

		_nearVerticesInCameraSpace = {
			glm::vec3{-x, -y, -_near},
			glm::vec3{x, -y, -_near},
			glm::vec3{-x, y, -_near},
			glm::vec3{x, y, -_near},
		};
		_farVerticesInCameraSpace = {
			glm::vec3{-x, -y, -_far},
			glm::vec3{x, -y, -_far},
			glm::vec3{-x, y, -_far},
			glm::vec3{x, y, -_far},
		};
	}
	// make screen y coordinate downwards
	//_perspectiveMatrix[1][1] *= -1;
}

void Camera::prepare()
{
	prepareDescriptorSet();
	onNodeUpdated();
}
void Camera::onNodeUpdated()
{
	updateProjectionMatrix();

	_cameraPara.P = getProjectionMatrix();
	_cameraPara.V = getViewMatrix();
	_cameraPara.eyePos = _parent->getGlobalPosition();

	_gpuBufferView.buffer->setData(_gpuBufferView.offset, sizeof(CameraPara), &_cameraPara);

	_updataSignal(this);
}
void Camera::updateGPUData(uint32_t frameIndex)
{
	Root::getSingleton().getDescriptorManager()->get(_setDataId)->upload(frameIndex);
}
void Camera::cullScene()
{
	auto scene = _parent->getParentScene();

	// find renderables
	std::vector<Renderable *> renderables;
	scene->getRootNode()->getComponents(renderables, true);
	_visibleRenderables.clear();
	for (auto p : renderables)
	{
		if (p->isEnable())
		{
			// TODO: frustum cull
			_visibleRenderables.emplace_back(p);
		}
	}

	// find lights
	_lights.clear();
	scene->getRootNode()->getComponents(_lights, true);
}
void Camera::render(uint32_t frameIndex)
{
	cullScene();
	_renderQueues->clear();
	_renderQueues->addRenderables(_visibleRenderables);
	_renderQueues->setLights(_lights);
	_renderQueues->preRender(frameIndex);
	_renderQueues->render(frameIndex);
	_renderQueues->postRender(frameIndex);
}
void Camera::setViewLayer(Shit::Extent2D ext, Shit::Format format)
{
	_viewLayer = std::make_unique<ViewLayer>(
		Shit::Extent2D{(uint32_t)(ext.width * _relativeViewport.extent.width),
					   (uint32_t)(ext.height * _relativeViewport.extent.height)},
		format);
}
void Camera::setCascadeSplitRatios(std::span<float const> ratios)
{
	_cascadeSplitRatios.clear();
	_cameraPara.cascadeNum = ratios.size() + 1;
	for (size_t i = 0; i < ratios.size(); ++i)
	{
		_cascadeSplitRatios.emplace_back(ratios[i]);
		_cameraPara.cascadeSplit[i] = (_far - _near) * ratios[i] + _near;
	}
}
std::array<glm::vec3, 8> Camera::getFrustumVertices(int cascadeIndex) const
{
	float start = 0;
	// float start = (cascadeIndex - 1) < 0 ? 0 : _cascadeSplitRatios[cascadeIndex - 1];
	float end = (cascadeIndex == _cascadeSplitRatios.size()) ? 1. : (_cascadeSplitRatios[cascadeIndex] + 0.01);

	std::array<glm::vec3, 8> ret;
	for (int i = 0; i < 4; ++i)
	{
		ret[i] = _nearVerticesInCameraSpace[i] + (_farVerticesInCameraSpace[i] - _nearVerticesInCameraSpace[i]) * start;
		ret[i + 4] = _nearVerticesInCameraSpace[i] + (_farVerticesInCameraSpace[i] - _nearVerticesInCameraSpace[i]) * end;
	}
	auto &&m = _parent->getGlobalTransformMatrix();
	for (auto &e : ret)
		e = glm::vec3(m * glm::vec4(e.x, e.y, e.z, 1));
	return ret;
}