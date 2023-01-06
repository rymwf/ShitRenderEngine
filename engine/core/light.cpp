#include "light.hpp"
#include "root.hpp"
#include "buffer.hpp"
#include "descriptorManager.hpp"
#include "material.hpp"
#include "sceneNode.hpp"
#include "camera.hpp"

Light::Light(SceneNode *parent, LightType lightType) : Component(parent), _shadowResolution({1024, 1024})
{
	_lightPara.lightType = lightType;
	switch (lightType)
	{
	case LIGHT_DIRECTIONAL:
		break;
	case LIGHT_POINT:
		break;
	case LIGHT_SPHERE:
		break;
	case LIGHT_SPOT:
		break;
	case LIGHT_TUBE:
		_lightPara.vertexNum = 2;
		break;
	case LIGHT_DISK_SINGLE_FACE:
		break;
	case LIGHT_QUAD_SINGLE_FACE:
		_lightPara.vertexNum = 4;
		break;
	case LIGHT_POLYGON_SINGLE_FACE:
		break;
	}
	createShadowTextures();
	createFramebuffers();
}
Light::Light(Light const &other) : Component(other)
{
	_enable = other._enable;
	_enableShadow = other._enableShadow;

	memcpy(&_lightPara, &other._lightPara, sizeof(LightPara));

	_shadowBias = other._shadowBias;
	_shadowNear = other._shadowNear;
	_shadowFar = other._shadowFar;
	_shadowRadius = other._shadowRadius;
	_shadowResolution = other._shadowResolution;

	// createViewLayer();
}
void Light::createShadowTextures()
{
	auto mgr = Root::getSingleton().getTextureManager();
	switch (_lightPara.lightType)
	{
	case LIGHT_DIRECTIONAL:
		// cascades, texture array 4
		{
			// _shadowTexture
			_shadowTexture = mgr->create(Shit::ImageCreateInfo{
											 {},
											 Shit::ImageType::TYPE_2D,
											 Shit::Format::D24_UNORM_S8_UINT,
											 {_shadowResolution.width, _shadowResolution.height, 1},
											 1,
											 4,
											 Shit::SampleCountFlagBits::BIT_1,
											 Shit::ImageTiling::OPTIMAL,
											 Shit::ImageUsageFlagBits::SAMPLED_BIT |
												 Shit::ImageUsageFlagBits::DEPTH_STENCIL_ATTACHMENT_BIT,
											 Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
											 Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
										 true, {TextureInterpolation::LINEAR, Shit::SamplerWrapMode::CLAMP_TO_EDGE, true});
		}
		break;
	case LIGHT_POINT:
	case LIGHT_SPHERE:
		// 1 texture cube
		{
			_shadowTexture = mgr->create(Shit::ImageCreateInfo{
											 Shit::ImageCreateFlagBits::CUBE_COMPATIBLE_BIT,
											 Shit::ImageType::TYPE_2D,
											 Shit::Format::D24_UNORM_S8_UINT,
											 {_shadowResolution.width, _shadowResolution.height, 1},
											 1,
											 6,
											 Shit::SampleCountFlagBits::BIT_1,
											 Shit::ImageTiling::OPTIMAL,
											 Shit::ImageUsageFlagBits::SAMPLED_BIT |
												 Shit::ImageUsageFlagBits::DEPTH_STENCIL_ATTACHMENT_BIT,
											 Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
											 Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
										 true, {TextureInterpolation::LINEAR, Shit::SamplerWrapMode::CLAMP_TO_EDGE, true});
		}
		break;
	case LIGHT_SPOT:
		// 1
		{
			_shadowTexture = mgr->create(Shit::ImageCreateInfo{
											 {},
											 Shit::ImageType::TYPE_2D,
											 Shit::Format::D24_UNORM_S8_UINT,
											 {_shadowResolution.width, _shadowResolution.height, 1},
											 1,
											 1,
											 Shit::SampleCountFlagBits::BIT_1,
											 Shit::ImageTiling::OPTIMAL,
											 Shit::ImageUsageFlagBits::SAMPLED_BIT |
												 Shit::ImageUsageFlagBits::DEPTH_STENCIL_ATTACHMENT_BIT,
											 Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
											 Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
										 true, {TextureInterpolation::LINEAR, Shit::SamplerWrapMode::CLAMP_TO_EDGE, true});
		}
		break;
	}
}
void Light::createFramebuffers()
{
	auto count = Root::getSingleton().getScreen()->getSwapchain()->GetImageCount();
	auto device = Root::getSingleton().getDevice();
	auto renderPass = Root::getSingleton().getRenderPass(RenderPassType::SHADOW);

	for (uint32_t i = 0; i < count; ++i)
	{
		Shit::ImageView *imageView;

		switch (_lightPara.lightType)
		{
		case LIGHT_DIRECTIONAL:
		case LIGHT_SPOT:
			imageView = _shadowTexture->getGpuImageView(
				Shit::ImageViewType::TYPE_2D_ARRAY,
				{Shit::ImageAspectFlagBits::DEPTH_BIT, 0, 1, 0, _shadowTexture->getImageCreateInfo()->arrayLayers},
				{}, i);
			break;
		case LIGHT_POINT:
		case LIGHT_SPHERE:
			imageView = _shadowTexture->getGpuImageView(
				Shit::ImageViewType::TYPE_CUBE,
				{Shit::ImageAspectFlagBits::DEPTH_BIT, 0, 1, 0, _shadowTexture->getImageCreateInfo()->arrayLayers},
				{}, i);
			break;
		}

		_shadowFramebuffers.emplace_back(device->Create(
			Shit::FramebufferCreateInfo{
				renderPass,
				1,
				&imageView,
				_shadowResolution,
				_shadowTexture->getImageCreateInfo()->arrayLayers,
			}));
	}
}
void Light::prepareDescriptorSet()
{
	auto buffer = Root::getSingleton().getBufferManager()->createOrRetriveBuffer(
		BufferPropertyDesciption{
			Shit::BufferUsageFlagBits::STORAGE_BUFFER_BIT,
			Shit::MemoryPropertyFlagBits::HOST_VISIBLE_BIT |
				Shit::MemoryPropertyFlagBits::HOST_COHERENT_BIT,
			true});

	_lightPara.transformMatrix = glm::mat4(1);

	_gpuBufferView = buffer->allocate(sizeof(LightPara), 1, &_lightPara);

	// create camera descriptor set
	auto setData = Root::getSingleton().getDescriptorManager()->createDescriptorSetData(
		Root::getSingleton().getDescriptorSetLayoutLight(), true);

	_setDataId = setData->getId();
	setData->setBuffer(DESCRIPTOR_BINDING_LIGHT, 0, _gpuBufferView);

	if (_lightPara.lightType == LIGHT_POINT || _lightPara.lightType == LIGHT_SPHERE)
	{
		setData->setTexture(
			9, 0,
			DescriptorTextureData{
				_shadowTexture,
				Shit::ImageViewType::TYPE_CUBE,
				_shadowTexture->getImageCreateInfo()->format,
				{},
				{Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, _shadowTexture->getImageCreateInfo()->arrayLayers},
				Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
				_shadowTexture->getSamplerInfo()});
	}
	else
	{
		setData->setTexture(
			8, 0,
			DescriptorTextureData{
				_shadowTexture,
				Shit::ImageViewType::TYPE_2D_ARRAY,
				_shadowTexture->getImageCreateInfo()->format,
				{},
				{Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, _shadowTexture->getImageCreateInfo()->arrayLayers},
				Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
				_shadowTexture->getSamplerInfo()});
	}
}
DescriptorSetData *Light::getDescriptorSetData() const
{
	return static_cast<DescriptorSetData *>(Root::getSingleton().getDescriptorManager()->get(_setDataId));
}
void Light::prepare()
{
	prepareDescriptorSet();
	onNodeUpdated();
}
void Light::onNodeUpdated()
{
	_lightPara.transformMatrix = _parent->getGlobalTransformMatrix();

	switch (_lightPara.lightType)
	{
	case LIGHT_DIRECTIONAL:
		break;
	case LIGHT_SPOT:
		break;
	case LIGHT_TUBE:
		_lightPara.radius = {glm::length(glm::mat3(_lightPara.transformMatrix) * glm::vec3(1, 0, 0)), 1, 1};
		_lightPara.vertices[0] = _lightPara.transformMatrix * glm::vec4(0, 0, 0, 1);
		_lightPara.vertices[1] = _lightPara.transformMatrix * glm::vec4(1, 0, 0, 1);
		break;
	case LIGHT_SPHERE:
		_lightPara.radius = {glm::length(glm::mat3(_lightPara.transformMatrix) * glm::vec3(1, 0, 0)), 1, 1};
		break;
	case LIGHT_DISK_SINGLE_FACE:
		_lightPara.radius = {
			glm::length(glm::mat3(_lightPara.transformMatrix) * glm::vec3(1, 0, 0)),
			1,
			glm::length(glm::mat3(_lightPara.transformMatrix) * glm::vec3(0, 0, 1))};
		break;
	case LIGHT_QUAD_SINGLE_FACE:
		_lightPara.radius = {
			glm::length(glm::mat3(_lightPara.transformMatrix) * glm::vec3(1, 0, 0)),
			1,
			glm::length(glm::mat3(_lightPara.transformMatrix) * glm::vec3(0, 0, 1))};
		_lightPara.vertices[0] = _lightPara.transformMatrix * glm::vec4(1, 0, 1, 1);
		_lightPara.vertices[1] = _lightPara.transformMatrix * glm::vec4(1, 0, -1, 1);
		_lightPara.vertices[2] = _lightPara.transformMatrix * glm::vec4(-1, 0, -1, 1);
		_lightPara.vertices[3] = _lightPara.transformMatrix * glm::vec4(-1, 0, 1, 1);
		break;
	}
	updateLightPVs();
	_gpuBufferView.buffer->setData(_gpuBufferView.offset, sizeof(LightPara), &_lightPara);
}
void Light::updateGPUData(uint32_t frameIndex)
{
	Root::getSingleton().getDescriptorManager()->get(_setDataId)->upload(frameIndex);
}
void Light::setCascadeFrustumVertices(std::span<glm::vec3 const> frustumVertices)
{
	_cascadeFrustumVertices.resize(frustumVertices.size());
	int i = 0;
	for (auto &&e : frustumVertices)
	{
		_cascadeFrustumVertices[i] = e;
		++i;
	}
	if (_lightPara.lightType == LIGHT_DIRECTIONAL)
		_parent->needUpdate();
}
void Light::updateLightPVs()
{
	switch (_lightPara.lightType)
	{
	case LIGHT_DIRECTIONAL:
	{
		// directional light
		glm::vec3 pmin = glm::vec3((std::numeric_limits<float>::max)());
		glm::vec3 pmax = glm::vec3((std::numeric_limits<float>::min)());
		auto m_v = glm::inverse(glm::mat3(_parent->getGlobalTransformMatrix()));
		glm::vec3 a;

		auto count = _cascadeFrustumVertices.size() / 8; // frustum count
		for (int i = 0; i < count; ++i)
		{
			pmin = glm::vec3((std::numeric_limits<float>::max)());
			pmax = glm::vec3((std::numeric_limits<float>::lowest)());
			for (int j = 0; j < 8; ++j)
			{
				auto &&e = _cascadeFrustumVertices[i * 8 + j];
				a = m_v * e;

				pmin = (glm::min)(a, pmin);
				pmax = (glm::max)(a, pmax);
			}
			_lightPara.P[i] = glm::orthoRH_ZO(pmin.x, pmax.x, pmin.y, pmax.y, -std::max(pmax.z, 100.f), -std::min(pmin.z, -100.f));
			_lightPara.V[i] = glm::mat4(m_v);
		}
	}
	break;
	case LIGHT_POINT:
	case LIGHT_SPHERE:
	{
		// point light
		auto lightPos = _parent->getGlobalPosition();
		const glm::mat4 M[]{
			glm::lookAtRH(lightPos, lightPos + glm::vec3{1, 0, 0}, {0, -1, 0}),
			glm::lookAtRH(lightPos, lightPos + glm::vec3{-1, 0, 0}, {0, -1, 0}),
			glm::lookAtRH(lightPos, lightPos + glm::vec3{0, 1, 0}, {0, 0, 1}),
			glm::lookAtRH(lightPos, lightPos + glm::vec3{0, -1, 0}, {0, 0, -1}),
			glm::lookAtRH(lightPos, lightPos + glm::vec3{0, 0, 1}, {0, -1, 0}),
			glm::lookAtRH(lightPos, lightPos + glm::vec3{0, 0, -1}, {0, -1, 0}),

		};
		const glm::mat4 P = glm::perspectiveRH_ZO(glm::radians(90.f), 1.f, 0.1f, _lightPara.rmax);
		for (int i = 0; i < 6; ++i)
		{
			_lightPara.P[i] = P;
			_lightPara.V[i] = M[i];
		}
	}
	break;
	case LIGHT_SPOT:
	{
		// spot light
		_lightPara.P[0] = glm::perspectiveRH_ZO(_lightPara.cosThetaU * 2.f, 1.f, 0.1f, _lightPara.rmax);
		_lightPara.V[0] = glm::inverse(_parent->getGlobalTransformMatrix());
	}
	break;
	default:
		break;
	}
}
void Light::cameraListener(Camera const *camera)
{
	std::vector<glm::vec3> vertices;
	for (int i = 0; i < camera->getCascadeNum(); ++i)
	{
		auto &&a = camera->getFrustumVertices(i);
		for (auto &&e : a)
			vertices.emplace_back(e);
	}
	setCascadeFrustumVertices(vertices);
}